/*
 * MIT License
 *
 * Copyright (c) 2026 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "AttachmentPool.h"
#include "Client.h"
#include "Exception.h"
#include <algorithm>
#include <cassert>
#include <exception>
#include <string_view>
#include <utility>

using namespace fbcpp;
using namespace fbcpp::impl;


static std::chrono::steady_clock::time_point steadyNow()
{
	return std::chrono::steady_clock::now();
}

static std::unique_ptr<AttachmentPoolEntry> extractEntry(
	std::vector<std::unique_ptr<AttachmentPoolEntry>>& entries, AttachmentPoolEntry* entry)
{
	const auto it = std::find_if(
		entries.begin(), entries.end(), [entry](const auto& candidate) { return candidate.get() == entry; });

	if (it == entries.end())
		return nullptr;

	auto owned = std::move(*it);
	entries.erase(it);

	return owned;
}


AttachmentPool::AttachmentPool(Client& client_, std::string uri_, const AttachmentPoolOptions& options)
	: client{&client_},
	  uri{std::move(uri_)},
	  options{options}
{
	if (options.getAttachmentOptions().getCreateDatabase())
	{
		throw FbCppException(
			"AttachmentPoolOptions::setAttachmentOptions must not enable AttachmentOptions::setCreateDatabase");
	}

	if (options.getMaxSize() < 1u)
		throw FbCppException("AttachmentPoolOptions::setMaxSize must be at least 1");

	if (options.getMinSize() > options.getMaxSize())
		throw FbCppException("AttachmentPoolOptions::setMinSize must not be greater than setMaxSize");

	for (std::size_t i = 0u; i < options.getMinSize(); ++i)
	{
		auto entry = std::make_unique<AttachmentPoolEntry>();
		entry->attachment = std::make_unique<Attachment>(*client, uri, options.getAttachmentOptions());
		entry->createdAt = entry->lastReturnedAt = steadyNow();

		available.push_back(entry.get());
		entries.push_back(std::move(entry));
	}
}

AttachmentPool::~AttachmentPool() noexcept
{
	assert(inUse == 0u && pending == 0u);

	try
	{
		close();
	}
	catch (...)
	{
		// swallow
	}
}

std::size_t AttachmentPool::size()
{
	std::lock_guard mutexGuard{mutex};
	return entries.size();
}

std::size_t AttachmentPool::availableCount()
{
	std::lock_guard mutexGuard{mutex};
	return available.size();
}

std::size_t AttachmentPool::inUseCount()
{
	std::lock_guard mutexGuard{mutex};
	return inUse;
}

PooledAttachment AttachmentPool::acquire()
{
	auto lease = acquireImpl(true);
	assert(lease.has_value());
	return std::move(*lease);
}

std::optional<PooledAttachment> AttachmentPool::tryAcquire()
{
	return acquireImpl(false);
}

void AttachmentPool::close()
{
	std::unique_lock mutexGuard{mutex};
	closed = true;

	while (!available.empty())
	{
		auto* entry = available.front();
		available.pop_front();

		auto owned = extractEntry(entries, entry);

		mutexGuard.unlock();
		owned.reset();  // Disconnects (network I/O) outside the lock.
		mutexGuard.lock();
	}
}

std::optional<PooledAttachment> AttachmentPool::acquireImpl(bool throwOnFailure)
{
	std::unique_lock mutexGuard{mutex};

	if (closed)
	{
		if (throwOnFailure)
			throw FbCppException("AttachmentPool is closed");

		return std::nullopt;
	}

	const auto deadline = steadyNow() + options.getAcquireTimeout();

	while (true)
	{
		evictLocked(mutexGuard);

		if (!available.empty())
		{
			auto* entry = available.front();
			available.pop_front();
			++inUse;

			if (options.getValidateOnAcquire())
			{
				mutexGuard.unlock();

				bool valid = true;

				try
				{
					entry->attachment->ping();
				}
				catch (...)
				{
					valid = false;
				}

				mutexGuard.lock();

				if (!valid)
				{
					--inUse;
					auto owned = extractEntry(entries, entry);

					mutexGuard.unlock();
					owned.reset();  // Disconnects (network I/O) outside the lock.
					mutexGuard.lock();

					continue;
				}
			}

			return PooledAttachment{*this, *entry};
		}

		if (entries.size() + pending < options.getMaxSize())
		{
			++pending;
			mutexGuard.unlock();

			std::unique_ptr<AttachmentPoolEntry> newEntry;
			std::exception_ptr failure;

			try
			{
				newEntry = std::make_unique<AttachmentPoolEntry>();
				newEntry->attachment = std::make_unique<Attachment>(*client, uri, options.getAttachmentOptions());
				newEntry->createdAt = newEntry->lastReturnedAt = steadyNow();
			}
			catch (...)
			{
				failure = std::current_exception();
			}

			mutexGuard.lock();
			--pending;

			if (failure)
			{
				availableCondition.notify_one();  // Let another waiter try instead.

				if (throwOnFailure)
					std::rethrow_exception(failure);

				return std::nullopt;
			}

			auto* entryPtr = newEntry.get();
			entries.push_back(std::move(newEntry));
			++inUse;

			return PooledAttachment{*this, *entryPtr};
		}

		const auto remaining = deadline - steadyNow();

		if (remaining <= std::chrono::steady_clock::duration::zero() ||
			availableCondition.wait_for(mutexGuard, remaining) == std::cv_status::timeout)
		{
			if (throwOnFailure)
				throw FbCppException("Timed out waiting for an available connection from AttachmentPool");

			return std::nullopt;
		}
	}
}

void AttachmentPool::evictLocked(std::unique_lock<std::mutex>& lock)
{
	assert(lock.owns_lock());

	const auto currentTime = steadyNow();
	std::vector<std::unique_ptr<AttachmentPoolEntry>> toDestroy;

	for (auto it = available.begin(); it != available.end();)
	{
		auto* entry = *it;
		bool evict = false;

		if (options.getMaxLifetime() && currentTime - entry->createdAt >= *options.getMaxLifetime())
			evict = true;
		else if (options.getIdleTimeout() && entries.size() > options.getMinSize() &&
			currentTime - entry->lastReturnedAt >= *options.getIdleTimeout())
			evict = true;

		if (evict)
		{
			it = available.erase(it);

			if (auto owned = extractEntry(entries, entry))
				toDestroy.push_back(std::move(owned));
		}
		else
			++it;
	}

	if (!toDestroy.empty())
	{
		lock.unlock();
		toDestroy.clear();  // Disconnects (network I/O) outside the lock.
		lock.lock();
	}
}

void AttachmentPool::release(AttachmentPoolEntry& entry) noexcept
{
	try
	{
		bool resetFailed = false;

		if (!closed && options.getSessionResetOnRelease() && entry.attachment && entry.attachment->isValid())
		{
			try
			{
				entry.attachment->resetSession();
			}
			catch (...)
			{
				resetFailed = true;  // Force this connection to be discarded below.
			}
		}

		std::unique_ptr<AttachmentPoolEntry> toDestroy;

		{  // scope
			std::lock_guard mutexGuard{mutex};

			assert(inUse > 0u);
			--inUse;

			const bool discard = closed || !entry.attachment || !entry.attachment->isValid() || resetFailed;

			if (discard)
				toDestroy = extractEntry(entries, &entry);
			else
			{
				entry.lastReturnedAt = steadyNow();
				available.push_back(&entry);
			}
		}

		availableCondition.notify_one();
		// toDestroy (if any) disconnects here, outside the lock.
	}
	catch (...)
	{
		// swallow
	}
}
