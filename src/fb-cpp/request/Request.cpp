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

#include "Request.h"
#include "../Attachment.h"
#include "../Transaction.h"
#include <algorithm>
#include <cassert>

namespace fbcpp::request
{
	Request::Request(Attachment& a, std::span<const std::byte> blr, const std::vector<MessageEntry>& entries)
		: attachment{&a},
		  statusWrapper{a.getClient()}
	{
		assert(attachment->isValid());

		for (const auto& entry : entries)
		{
			if (entry.messageNumber > 255u)
				throw FbCppException("Message number must be between 0 and 255");

			const auto it = std::lower_bound(messages.begin(), messages.end(), entry.messageNumber,
				[](const auto& pair, unsigned number) { return pair.first < number; });

			if (it != messages.end() && it->first == entry.messageNumber)
				throw FbCppException("Duplicated message number " + std::to_string(entry.messageNumber));

			messages.emplace(it, entry.messageNumber, RequestMessage{attachment->getClient(), entry.format});
		}

		handle.reset(attachment->getHandle()->compileRequest(
			&statusWrapper, static_cast<unsigned>(blr.size()), reinterpret_cast<const unsigned char*>(blr.data())));
	}

	Request::Request(Request&& o) noexcept
		: attachment{o.attachment},
		  messages{std::move(o.messages)},
		  statusWrapper{std::move(o.statusWrapper)},
		  handle{std::move(o.handle)}
	{
	}

	Request& Request::operator=(Request&& o) noexcept
	{
		if (this != &o)
		{
			attachment = o.attachment;
			messages = std::move(o.messages);
			statusWrapper = std::move(o.statusWrapper);
			handle = std::move(o.handle);
		}

		return *this;
	}

	RequestMessage& Request::getMessage(unsigned messageNumber)
	{
		const auto message = findMessage(messageNumber);

		if (!message)
		{
			throw FbCppException(
				"Message number " + std::to_string(messageNumber) + " was not declared in the request");
		}

		return *message;
	}

	void Request::free()
	{
		assert(isValid());

		handle->free(&statusWrapper);
		handle.reset();
	}

	void Request::start(Transaction& transaction, unsigned level)
	{
		assert(isValid());
		assert(transaction.isValid());

		handle->start(&statusWrapper, transaction.getHandle().get(), static_cast<int>(level));
	}

	void Request::startAndSend(Transaction& transaction, unsigned messageNumber, unsigned level)
	{
		assert(isValid());
		assert(transaction.isValid());

		const auto message = findMessage(messageNumber);

		if (!message)
		{
			throw FbCppException(
				"Message number " + std::to_string(messageNumber) + " was not declared in the request");
		}

		handle->startAndSend(&statusWrapper, transaction.getHandle().get(), static_cast<int>(level), messageNumber,
			static_cast<unsigned>(message->message.size()), message->message.data());
	}

	void Request::send(unsigned messageNumber, unsigned level)
	{
		assert(isValid());

		const auto message = findMessage(messageNumber);

		if (!message)
		{
			throw FbCppException(
				"Message number " + std::to_string(messageNumber) + " was not declared in the request");
		}

		handle->send(&statusWrapper, static_cast<int>(level), messageNumber,
			static_cast<unsigned>(message->message.size()), message->message.data());
	}

	bool Request::receive(unsigned messageNumber, unsigned level)
	{
		assert(isValid());

		const auto message = findMessage(messageNumber);

		if (!message)
		{
			throw FbCppException(
				"Message number " + std::to_string(messageNumber) + " was not declared in the request");
		}

		try
		{
			handle->receive(&statusWrapper, static_cast<int>(level), messageNumber,
				static_cast<unsigned>(message->message.size()), message->message.data());
		}
		catch (const DatabaseException& e)
		{
			if (e.getErrorCode() == isc_req_sync)  // end of records
				return false;

			throw;
		}

		return true;
	}

	void Request::unwind(unsigned level)
	{
		assert(isValid());

		handle->unwind(&statusWrapper, static_cast<int>(level));
	}

	RequestMessage* Request::findMessage(unsigned messageNumber) const noexcept
	{
		const auto it = std::lower_bound(messages.begin(), messages.end(), messageNumber,
			[](const auto& pair, unsigned number) { return pair.first < number; });

		if (it != messages.end() && it->first == messageNumber)
			return const_cast<RequestMessage*>(&it->second);

		return nullptr;
	}
}  // namespace fbcpp::request
