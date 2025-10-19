/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to do so, subject to the
 * following conditions:
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

#include "Blob.h"
#include "Attachment.h"
#include "Client.h"
#include "Transaction.h"
#include "fb-api/firebird/impl/inf_pub.h"
#include <limits>

using namespace fbcpp;
using namespace fbcpp::impl;

namespace
{
	std::pair<unsigned, const unsigned char*> getBpb(const BlobOptions& options)
	{
		const auto& bpb = options.getBpb();

		if (bpb.size() > std::numeric_limits<unsigned>::max())
			throw FbCppException("BPB too large");

		const auto length = static_cast<unsigned>(bpb.size());
		const auto data = bpb.empty() ? nullptr : reinterpret_cast<const unsigned char*>(bpb.data());

		return {length, data};
	}
}  // namespace


Blob::Blob(Attachment& attachment, Transaction& transaction, const BlobOptions& options)
	: attachment{attachment},
	  transaction{transaction},
	  status{attachment.getClient().newStatus()},
	  statusWrapper{attachment.getClient(), status.get()}
{
	assert(attachment.isValid());
	assert(transaction.isValid());

	const auto [bpbLength, bpbData] = getBpb(options);

	handle.reset(
		attachment.getHandle()->createBlob(&statusWrapper, transaction.getHandle().get(), &id.id, bpbLength, bpbData));
}

Blob::Blob(Attachment& attachment, Transaction& transaction, const BlobId& blobId, const BlobOptions& options)
	: attachment{attachment},
	  transaction{transaction},
	  id{blobId},
	  status{attachment.getClient().newStatus()},
	  statusWrapper{attachment.getClient(), status.get()}
{
	assert(attachment.isValid());
	assert(transaction.isValid());

	const auto [bpbLength, bpbData] = getBpb(options);

	handle.reset(
		attachment.getHandle()->openBlob(&statusWrapper, transaction.getHandle().get(), &id.id, bpbLength, bpbData));
}

unsigned Blob::getLength()
{
	assert(isValid());

	const unsigned char items[] = {isc_info_blob_total_length};
	unsigned char buffer[16]{};

	handle->getInfo(&statusWrapper, sizeof(items), items, sizeof(buffer), buffer);

	const auto* ptr = buffer;
	const auto* end = buffer + sizeof(buffer);

	while (ptr < end)
	{
		const auto item = *ptr++;

		if (item == isc_info_end)
			break;

		if (item == isc_info_truncated)
			throw FbCppException("Blob::getLength truncated response");

		if (item == isc_info_error)
			throw FbCppException("Blob::getLength error response");

		if (ptr + 2 > end)
			throw FbCppException("Blob::getLength malformed response");

		const auto itemLength = static_cast<std::uint16_t>((ptr[0]) | (ptr[1] << 8));
		ptr += 2;

		if (ptr + itemLength > end)
			throw FbCppException("Blob::getLength invalid length");

		if (item == isc_info_blob_total_length)
		{
			unsigned result = 0;

			for (std::uint16_t i = 0; i < itemLength; ++i)
				result |= static_cast<unsigned>(ptr[i]) << (8u * i);

			return result;
		}

		ptr += itemLength;
	}

	throw FbCppException("Blob::getLength value not found");
}

unsigned Blob::read(std::span<std::byte> buffer)
{
	assert(isValid());

	unsigned totalRead = 0;
	const unsigned maxChunkSize = std::numeric_limits<std::uint16_t>::max();

	while (!buffer.empty())
	{
		const auto chunkSize = buffer.size() < maxChunkSize ? buffer.size() : maxChunkSize;
		const auto chunk = buffer.first(chunkSize);
		const auto readNow = readSegment(chunk);

		if (readNow == 0)
			break;

		totalRead += readNow;
		buffer = buffer.subspan(readNow);
	}

	return totalRead;
}

unsigned Blob::readSegment(std::span<std::byte> buffer)
{
	assert(isValid());

	if (buffer.empty())
		return 0;

	unsigned segmentLength = 0;
	const auto result =
		handle->getSegment(&statusWrapper, static_cast<unsigned>(buffer.size()), buffer.data(), &segmentLength);

	switch (result)
	{
		case fb::IStatus::RESULT_OK:
		case fb::IStatus::RESULT_SEGMENT:
			return segmentLength;

		case fb::IStatus::RESULT_NO_DATA:
			return 0;

		default:
			return 0;
	}
}

void Blob::write(std::span<const std::byte> buffer)
{
	assert(isValid());

	const unsigned maxChunkSize = std::numeric_limits<std::uint16_t>::max();

	while (!buffer.empty())
	{
		const auto chunkSize = buffer.size() < maxChunkSize ? buffer.size() : maxChunkSize;
		const auto chunk = buffer.first(chunkSize);
		writeSegment(chunk);
		buffer = buffer.subspan(chunkSize);
	}
}

void Blob::writeSegment(std::span<const std::byte> buffer)
{
	assert(isValid());

	if (buffer.empty())
		return;

	if (buffer.size() > std::numeric_limits<unsigned>::max())
		throw FbCppException("Segment too large");

	handle->putSegment(&statusWrapper, static_cast<unsigned>(buffer.size()), buffer.data());
}

int Blob::seek(BlobSeekMode mode, int offset)
{
	assert(isValid());
	return handle->seek(&statusWrapper, static_cast<int>(mode), offset);
}

void Blob::cancel()
{
	assert(isValid());

	handle->cancel(&statusWrapper);
	handle.reset();
}

void Blob::close()
{
	assert(isValid());

	handle->close(&statusWrapper);
	handle.reset();
}
