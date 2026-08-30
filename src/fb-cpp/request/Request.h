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

#ifndef FBCPP_REQUEST_H
#define FBCPP_REQUEST_H

#include "fb-cpp-export.h"
#include "../fb-api.h"
#include "../Exception.h"
#include "RequestMessage.h"
#include "RequestMessageFormat.h"
#include "../SmartPtrs.h"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Attachment;
	class Transaction;
}  // namespace fbcpp


///
/// fb-cpp request namespace.
///
namespace fbcpp::request
{
	///
	/// Represents a Firebird request: a precompiled BLR (Binary Language Representation) program.
	///
	/// Requests bypass SQL entirely. The BLR is supplied by the caller - typically produced by gpre or hand-written -
	/// and declares its own messages with `blr_message` clauses. Describe each message you intend to access with a
	/// RequestMessageFormat (preferably generating its clause with buildBlrMessageClause) and pass both to the
	/// constructor; values are then read and written through typed accessors instead of raw buffers.
	///
	/// A request is executed with start or startAndSend. Records produced by the BLR body (`blr_send` nodes) are
	/// consumed with receive, which returns false when the request is finished. Input messages declared by the BLR body
	/// (`blr_receive` nodes) are delivered with send.
	///
	class FB_CPP_EXPORT Request final
	{
	public:
		///
		/// Associates a message number declared in the BLR with its format.
		///
		struct MessageEntry final
		{
			///
			/// The message number, between 0 and 255.
			///
			unsigned messageNumber;

			///
			/// The message format.
			///
			RequestMessageFormat format;
		};

	public:
		///
		/// Compiles a request from the given BLR for the attachment's database.
		///
		explicit Request(
			Attachment& attachment, std::span<const std::byte> blr, const std::vector<MessageEntry>& messages = {});

		///
		/// Move constructor.
		/// A moved Request object becomes invalid.
		///
		Request(Request&& o) noexcept;

		///
		/// Move assignment.
		/// A moved Request object becomes invalid.
		///
		Request& operator=(Request&& o) noexcept;

		Request(const Request&) = delete;
		Request& operator=(const Request&) = delete;

		///
		/// Releases resources; ignores failures to keep destructor noexcept.
		///
		~Request() noexcept
		{
			if (isValid())
			{
				try
				{
					free();
				}
				catch (...)
				{
					// swallow
				}
			}
		}

	public:
		///
		/// Returns whether the Request object is valid.
		///
		bool isValid() noexcept
		{
			return handle != nullptr;
		}

		///
		/// Returns the Attachment object reference used to create this Request.
		///
		Attachment& getAttachment() noexcept
		{
			return *attachment;
		}

		///
		/// Returns the internal Firebird IRequest handle.
		///
		FbRef<fb::IRequest> getHandle() noexcept
		{
			return handle;
		}

		///
		/// Returns the message associated with a message number declared at construction time.
		///
		RequestMessage& getMessage(unsigned messageNumber);

		///
		/// Releases the compiled request.
		///
		void free();

		///
		/// Starts the execution of the request.
		///
		void start(Transaction& transaction, unsigned level = 0);

		///
		/// Starts the execution of the request, sending it an input message.
		///
		void startAndSend(Transaction& transaction, unsigned messageNumber, unsigned level = 0);

		///
		/// Sends an input message to the running request.
		///
		void send(unsigned messageNumber, unsigned level = 0);

		///
		/// Receives a record from the running request into the associated message.
		/// Returns false when the request has no more records.
		///
		bool receive(unsigned messageNumber, unsigned level = 0);

		///
		/// Unwinds the running request.
		///
		void unwind(unsigned level = 0);

	private:
		RequestMessage* findMessage(unsigned messageNumber) const noexcept;

	private:
		Attachment* attachment;
		std::vector<std::pair<unsigned, RequestMessage>> messages;
		impl::StatusWrapper statusWrapper;
		FbRef<fb::IRequest> handle;
	};
}  // namespace fbcpp::request


#endif  // FBCPP_REQUEST_H
