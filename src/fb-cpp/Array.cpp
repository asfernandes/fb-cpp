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

#include "Array.h"
#include "Attachment.h"
#include "Exception.h"
#include "Transaction.h"
#include "firebird/impl/blr.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

using namespace fbcpp;
using namespace fbcpp::impl;


static unsigned char arrayElementType(DescriptorOriginalType type)
{
	switch (type)
	{
		case DescriptorOriginalType::TEXT:
			return blr_text;

		case DescriptorOriginalType::VARYING:
			return blr_varying;

		case DescriptorOriginalType::SHORT:
			return blr_short;

		case DescriptorOriginalType::LONG:
			return blr_long;

		case DescriptorOriginalType::FLOAT:
			return blr_float;

		case DescriptorOriginalType::DOUBLE:
			return blr_double;

		case DescriptorOriginalType::TIMESTAMP:
			return blr_timestamp;

		case DescriptorOriginalType::TIME:
			return blr_sql_time;

		case DescriptorOriginalType::DATE:
			return blr_sql_date;

		case DescriptorOriginalType::INT64:
			return blr_int64;

		case DescriptorOriginalType::TIMESTAMP_TZ:
			return blr_timestamp_tz;

		case DescriptorOriginalType::TIMESTAMP_TZ_EX:
			return blr_ex_timestamp_tz;

		case DescriptorOriginalType::TIME_TZ:
			return blr_sql_time_tz;

		case DescriptorOriginalType::TIME_TZ_EX:
			return blr_ex_time_tz;

		case DescriptorOriginalType::INT128:
			return blr_int128;

		case DescriptorOriginalType::DEC16:
			return blr_dec64;

		case DescriptorOriginalType::DEC34:
			return blr_dec128;

		case DescriptorOriginalType::BOOLEAN:
			return blr_bool;

		default:
			throw FbCppException("Unsupported Firebird array element type");
	}
}


Array::Array(Attachment& attachment, Transaction& transaction, ArrayDescriptor descriptor, ArrayId id)
	: attachment{attachment},
	  transaction{transaction},
	  descriptor{std::move(descriptor)},
	  id{id}
{
	assert(attachment.isValid());
	assert(transaction.isValid());
	buildIscDescriptor();
}

std::size_t Array::getSliceLength() const
{
	std::size_t result = descriptor.elementLength;

	for (const auto& bound : descriptor.bounds)
	{
		const auto length = static_cast<unsigned>(bound.upper) - static_cast<unsigned>(bound.lower) + 1u;

		if (result > std::numeric_limits<std::size_t>::max() / length)
			throw FbCppException("Firebird array slice is too large");

		result *= length;
	}

	return result;
}

unsigned Array::read(std::span<std::byte> buffer)
{
	validateBuffer(buffer.size());

	std::array<ISC_UCHAR, 1024> sdl{};
	ISC_SHORT sdlBufferLength = static_cast<ISC_SHORT>(sdl.size());
	ISC_SHORT sdlLength = 0;
	const auto iscDescriptor = buildIscDescriptor();
	std::array<ISC_STATUS, 20> statusVector{};
	const auto status =
		isc_array_gen_sdl(statusVector.data(), &iscDescriptor, &sdlBufferLength, sdl.data(), &sdlLength);

	if (status != 0)
		throw FbCppException("Cannot generate Firebird array SDL: " + std::to_string(status));

	StatusWrapper statusWrapper{attachment.getClient()};
	const auto result = attachment.getHandle()->getSlice(&statusWrapper, transaction.getHandle().get(), &id.id,
		static_cast<unsigned>(sdlLength), sdl.data(), 0, nullptr, static_cast<int>(buffer.size()),
		reinterpret_cast<unsigned char*>(buffer.data()));

	return static_cast<unsigned>(result);
}

void Array::write(std::span<const std::byte> buffer)
{
	validateBuffer(buffer.size());

	std::array<ISC_UCHAR, 1024> sdl{};
	ISC_SHORT sdlBufferLength = static_cast<ISC_SHORT>(sdl.size());
	ISC_SHORT sdlLength = 0;
	const auto iscDescriptor = buildIscDescriptor();
	std::array<ISC_STATUS, 20> statusVector{};
	const auto status =
		isc_array_gen_sdl(statusVector.data(), &iscDescriptor, &sdlBufferLength, sdl.data(), &sdlLength);

	if (status != 0)
		throw FbCppException("Cannot generate Firebird array SDL: " + std::to_string(status));

	StatusWrapper statusWrapper{attachment.getClient()};
	attachment.getHandle()->putSlice(&statusWrapper, transaction.getHandle().get(), &id.id,
		static_cast<unsigned>(sdlLength), sdl.data(), 0, nullptr, static_cast<int>(buffer.size()),
		reinterpret_cast<unsigned char*>(const_cast<std::byte*>(buffer.data())));
}

void Array::validateBuffer(std::size_t size) const
{
	if (size != getSliceLength())
		throw FbCppException("Firebird array buffer size does not match the complete slice length");
}

ISC_ARRAY_DESC Array::buildIscDescriptor() const
{
	if (descriptor.bounds.empty() || descriptor.bounds.size() > 16u)
		throw FbCppException("Firebird arrays must have between 1 and 16 dimensions");

	if (descriptor.elementLength == 0 || descriptor.elementLength > std::numeric_limits<unsigned short>::max())
		throw FbCppException("Invalid Firebird array element length");

	if (descriptor.scale < std::numeric_limits<ISC_SCHAR>::min() ||
		descriptor.scale > std::numeric_limits<ISC_SCHAR>::max())
	{
		throw FbCppException("Invalid Firebird array element scale");
	}

	ISC_ARRAY_DESC result{};

	if (descriptor.relation.size() >= sizeof(result.array_desc_relation_name) ||
		descriptor.field.size() >= sizeof(result.array_desc_field_name))
	{
		throw FbCppException("Firebird array relation or field name is too long");
	}

	std::memcpy(result.array_desc_relation_name, descriptor.relation.data(), descriptor.relation.size());
	std::memcpy(result.array_desc_field_name, descriptor.field.data(), descriptor.field.size());
	result.array_desc_dtype = arrayElementType(descriptor.elementType);
	result.array_desc_scale = static_cast<ISC_SCHAR>(descriptor.scale);
	result.array_desc_length = static_cast<unsigned short>(descriptor.elementLength);
	result.array_desc_dimensions = static_cast<short>(descriptor.bounds.size());

	for (std::size_t index = 0; index < descriptor.bounds.size(); ++index)
	{
		const auto& bound = descriptor.bounds[index];

		if (bound.lower > bound.upper)
			throw FbCppException("Firebird array lower bound exceeds upper bound");

		result.array_desc_bounds[index] = {bound.lower, bound.upper};
	}

	return result;
}
