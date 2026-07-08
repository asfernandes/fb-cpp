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

#ifndef FBCPP_ATTACHMENT_POOL_H
#define FBCPP_ATTACHMENT_POOL_H

#include "Attachment.h"
#include "fb-api.h"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Client;

	///
	/// Represents options used when creating an AttachmentPool object.
	///
	class AttachmentPoolOptions final
	{
	public:
		///
		/// Returns the options used to create each pooled Attachment.
		///
		const AttachmentOptions& getAttachmentOptions() const
		{
			return attachmentOptions;
		}

		///
		/// Sets the options used to create each pooled Attachment.
		/// AttachmentOptions::setCreateDatabase must not be enabled, since a pool only attaches to an
		/// existing database.
		///
		AttachmentPoolOptions& setAttachmentOptions(const AttachmentOptions& value)
		{
			attachmentOptions = value;
			return *this;
		}

		///
		/// Returns the minimum number of connections kept warm by the pool.
		///
		std::size_t getMinSize() const
		{
			return minSize;
		}

		///
		/// Sets the minimum number of connections kept warm by the pool.
		///
		AttachmentPoolOptions& setMinSize(std::size_t value)
		{
			minSize = value;
			return *this;
		}

		///
		/// Returns the maximum number of connections the pool may create.
		///
		std::size_t getMaxSize() const
		{
			return maxSize;
		}

		///
		/// Sets the maximum number of connections the pool may create.
		///
		AttachmentPoolOptions& setMaxSize(std::size_t value)
		{
			maxSize = value;
			return *this;
		}

		///
		/// Returns how long acquire() waits for a connection to become available before throwing.
		///
		std::chrono::milliseconds getAcquireTimeout() const
		{
			return acquireTimeout;
		}

		///
		/// Sets how long acquire() waits for a connection to become available before throwing.
		///
		AttachmentPoolOptions& setAcquireTimeout(std::chrono::milliseconds value)
		{
			acquireTimeout = value;
			return *this;
		}

		///
		/// Returns the maximum time a connection may stay idle in the pool before being closed.
		///
		const std::optional<std::chrono::milliseconds>& getIdleTimeout() const
		{
			return idleTimeout;
		}

		///
		/// Sets the maximum time a connection may stay idle in the pool before being closed.
		/// Connections are never evicted below getMinSize() due to idling.
		///
		AttachmentPoolOptions& setIdleTimeout(std::chrono::milliseconds value)
		{
			idleTimeout = value;
			return *this;
		}

		///
		/// Returns the maximum lifetime of a connection, regardless of usage.
		///
		const std::optional<std::chrono::milliseconds>& getMaxLifetime() const
		{
			return maxLifetime;
		}

		///
		/// Sets the maximum lifetime of a connection, regardless of usage.
		///
		AttachmentPoolOptions& setMaxLifetime(std::chrono::milliseconds value)
		{
			maxLifetime = value;
			return *this;
		}

		///
		/// Returns whether a connection is validated (via ping) before being handed out by acquire().
		///
		bool getValidateOnAcquire() const
		{
			return validateOnAcquire;
		}

		///
		/// Sets whether a connection is validated (via ping) before being handed out by acquire().
		///
		AttachmentPoolOptions& setValidateOnAcquire(bool value)
		{
			validateOnAcquire = value;
			return *this;
		}

		///
		/// Returns whether a connection has its session reset (via ALTER SESSION RESET) before being
		/// returned to the pool.
		///
		bool getSessionResetOnRelease() const
		{
			return sessionResetOnRelease;
		}

		///
		/// Sets whether a connection has its session reset (via ALTER SESSION RESET) before being
		/// returned to the pool. A connection whose reset fails is discarded instead of being reused.
		///
		AttachmentPoolOptions& setSessionResetOnRelease(bool value)
		{
			sessionResetOnRelease = value;
			return *this;
		}

	private:
		AttachmentOptions attachmentOptions;
		std::size_t minSize = 0u;
		std::size_t maxSize = 10u;
		std::chrono::milliseconds acquireTimeout{30000};
		std::optional<std::chrono::milliseconds> idleTimeout;
		std::optional<std::chrono::milliseconds> maxLifetime;
		bool validateOnAcquire = true;
		bool sessionResetOnRelease = false;
	};


	namespace impl
	{
		///
		/// Internal state tracked by AttachmentPool for a single pooled connection.
		/// This is an implementation detail shared by AttachmentPool and PooledAttachment.
		///
		struct AttachmentPoolEntry final
		{
			std::unique_ptr<Attachment> attachment;
			std::chrono::steady_clock::time_point createdAt;
			std::chrono::steady_clock::time_point lastReturnedAt;
		};
	}  // namespace impl


	class AttachmentPool;

	///
	/// RAII lease for an Attachment borrowed from an AttachmentPool.
	/// The leased Attachment is confined to the thread holding the lease; it is not thread-safe.
	/// Returns the Attachment to the pool when the lease is destroyed, unless it was already released.
	///
	class PooledAttachment final
	{
		friend class AttachmentPool;

	private:
		explicit PooledAttachment(AttachmentPool& pool, impl::AttachmentPoolEntry& entry) noexcept
			: pool{&pool},
			  entry{&entry}
		{
		}

	public:
		///
		/// Move constructor.
		/// A moved PooledAttachment object becomes invalid.
		///
		PooledAttachment(PooledAttachment&& o) noexcept
			: pool{o.pool},
			  entry{o.entry}
		{
			o.pool = nullptr;
			o.entry = nullptr;
		}

		///
		/// Move assignment.
		/// Releases the currently held Attachment (if any) back to the pool before taking ownership of o's.
		///
		PooledAttachment& operator=(PooledAttachment&& o) noexcept
		{
			if (this != &o)
			{
				release();

				pool = o.pool;
				entry = o.entry;
				o.pool = nullptr;
				o.entry = nullptr;
			}

			return *this;
		}

		PooledAttachment(const PooledAttachment&) = delete;
		PooledAttachment& operator=(const PooledAttachment&) = delete;

		///
		/// Returns the Attachment to the pool, unless it was already released.
		///
		~PooledAttachment() noexcept
		{
			try
			{
				release();
			}
			catch (...)
			{
				// swallow
			}
		}

	public:
		///
		/// Returns whether this object currently holds a leased Attachment.
		///
		bool isValid() const noexcept
		{
			return entry != nullptr;
		}

		///
		/// Returns the leased Attachment.
		///
		Attachment& get() noexcept
		{
			assert(entry);
			return *entry->attachment;
		}

		Attachment& operator*() noexcept
		{
			return get();
		}

		Attachment* operator->() noexcept
		{
			return &get();
		}

		///
		/// Returns the leased Attachment to the pool immediately.
		/// Does nothing if the lease was already released.
		///
		void release();

	private:
		AttachmentPool* pool;
		impl::AttachmentPoolEntry* entry;
	};

	///
	/// Represents a thread-safe pool of Attachment objects connected to the same database.
	/// The pool itself is thread-safe, but each leased Attachment (and any Transaction or Statement
	/// created from it) must be used by a single thread at a time.
	/// The Client and the AttachmentPool must exist and remain valid while there are outstanding
	/// PooledAttachment leases.
	///
	class AttachmentPool final
	{
		friend class PooledAttachment;

	public:
		///
		/// Constructs an AttachmentPool that connects to the database specified by the URI using the
		/// specified Client object and options.
		///
		explicit AttachmentPool(Client& client, std::string uri, const AttachmentPoolOptions& options = {});

		///
		/// Closes every idle connection in the pool.
		/// All leases must be returned to the pool before it is destroyed.
		///
		~AttachmentPool() noexcept;

		AttachmentPool(const AttachmentPool&) = delete;
		AttachmentPool& operator=(const AttachmentPool&) = delete;

		AttachmentPool(AttachmentPool&&) = delete;
		AttachmentPool& operator=(AttachmentPool&&) = delete;

	public:
		///
		/// Returns the Client object reference used to create this AttachmentPool object.
		///
		Client& getClient() noexcept
		{
			return *client;
		}

		///
		/// Returns the database URI this pool connects to.
		///
		const std::string& getUri() const noexcept
		{
			return uri;
		}

		///
		/// Returns the options used to create this AttachmentPool object.
		///
		const AttachmentPoolOptions& getOptions() const noexcept
		{
			return options;
		}

		///
		/// Returns the total number of connections currently created by the pool (in use plus available).
		///
		std::size_t size();

		///
		/// Returns the number of connections currently available (not leased) in the pool.
		///
		std::size_t availableCount();

		///
		/// Returns the number of connections currently leased out by the pool.
		///
		std::size_t inUseCount();

		///
		/// Acquires a connection from the pool, creating one if needed and allowed by getMaxSize().
		/// Blocks up to getAcquireTimeout() while the pool is exhausted, then throws FbCppException.
		///
		PooledAttachment acquire();

		///
		/// Attempts to acquire a connection from the pool without throwing.
		/// Returns std::nullopt if none becomes available within getAcquireTimeout().
		///
		std::optional<PooledAttachment> tryAcquire();

		///
		/// Closes every available connection and marks the pool as closed.
		/// Further calls to acquire() or tryAcquire() will fail.
		///
		void close();

	private:
		std::optional<PooledAttachment> acquireImpl(bool throwOnFailure);
		void evictLocked(std::unique_lock<std::mutex>& lock);
		void release(impl::AttachmentPoolEntry& entry) noexcept;

	private:
		Client* client;
		std::string uri;
		AttachmentPoolOptions options;
		std::vector<std::unique_ptr<impl::AttachmentPoolEntry>> entries;
		std::deque<impl::AttachmentPoolEntry*> available;
		std::size_t inUse = 0u;
		std::size_t pending = 0u;
		bool closed = false;
		std::mutex mutex;
		std::condition_variable availableCondition;
	};

	inline void PooledAttachment::release()
	{
		if (!entry)
			return;

		const auto poolPtr = pool;
		const auto entryPtr = entry;

		pool = nullptr;
		entry = nullptr;

		poolPtr->release(*entryPtr);
	}
}  // namespace fbcpp


#endif  // FBCPP_ATTACHMENT_POOL_H
