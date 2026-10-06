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
#include "firebird/impl/inf_pub.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

using namespace fbcpp;
using namespace fbcpp::impl;

Blob::Blob(Attachment& attachment, Transaction& transaction, const BlobOptions& options)
	: attachment{attachment},
	  transaction{transaction},
	  statusWrapper{attachment.getClient()}
{
	assert(attachment.isValid());
	assert(transaction.isValid());

	const auto preparedBpb = prepareBpb(options);

	handle.reset(attachment.getHandle()->createBlob(&statusWrapper, transaction.getHandle().get(), &id.id,
		static_cast<unsigned>(preparedBpb.size()), preparedBpb.data()));
}

Blob::Blob(Attachment& attachment, Transaction& transaction, const BlobId& blobId, const BlobOptions& options)
	: attachment{attachment},
	  transaction{transaction},
	  id{blobId},
	  statusWrapper{attachment.getClient()}
{
	assert(attachment.isValid());
	assert(transaction.isValid());

	const auto preparedBpb = prepareBpb(options);

	handle.reset(attachment.getHandle()->openBlob(&statusWrapper, transaction.getHandle().get(), &id.id,
		static_cast<unsigned>(preparedBpb.size()), preparedBpb.data()));
}

std::vector<std::uint8_t> Blob::prepareBpb(const BlobOptions& options)
{
	// IXpbBuilder::BPB only exists since Firebird 4, so the BPB is built by hand to work with Firebird 3 clients.

	const auto& userBpb = options.getBpb();
	std::vector<std::uint8_t> bpb(userBpb.begin(), userBpb.end());

	const auto addInt = [&](std::uint8_t tag, int value)
	{
		if (bpb.empty())
			bpb.push_back(isc_bpb_version1);

		const auto unsignedValue = static_cast<unsigned>(value);

		bpb.push_back(tag);
		bpb.push_back(4u);

		for (unsigned shift = 0u; shift < 32u; shift += 8u)
			bpb.push_back(static_cast<std::uint8_t>((unsignedValue >> shift) & 0xFFu));
	};

	if (const auto type = options.getType(); type.has_value())
		addInt(isc_bpb_type, static_cast<int>(type.value()));

	if (const auto sourceType = options.getSourceType(); sourceType.has_value())
		addInt(isc_bpb_source_type, static_cast<int>(sourceType.value()));

	if (const auto targetType = options.getTargetType(); targetType.has_value())
		addInt(isc_bpb_target_type, static_cast<int>(targetType.value()));

	if (const auto sourceCharSet = options.getSourceCharSet(); sourceCharSet.has_value())
		addInt(isc_bpb_source_interp, static_cast<int>(sourceCharSet.value()));

	if (const auto targetCharSet = options.getTargetCharSet(); targetCharSet.has_value())
		addInt(isc_bpb_target_interp, static_cast<int>(targetCharSet.value()));

	if (const auto storage = options.getStorage(); storage.has_value())
		addInt(isc_bpb_storage, static_cast<int>(storage.value()));

	return bpb;
}

unsigned Blob::getLength()
{
	assert(isValid());

	const std::uint8_t items[] = {isc_info_blob_total_length};
	std::uint8_t buffer[16]{};

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
	handle.release();
}

void Blob::close()
{
	assert(isValid());

	handle->close(&statusWrapper);
	handle.release();
}
