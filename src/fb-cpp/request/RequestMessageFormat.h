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

#ifndef FBCPP_REQUEST_MESSAGE_FORMAT_H
#define FBCPP_REQUEST_MESSAGE_FORMAT_H

#include "../config.h"
#include "fb-cpp-export.h"
#include "../fb-api.h"
#include "../types.h"
#include "../Descriptor.h"
#include "../Exception.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
#include <boost/multiprecision/cpp_int.hpp>
#endif


///
/// fb-cpp request namespace.
///
namespace fbcpp::request
{
	///
	/// Describes the fields of a Firebird request message.
	///
	/// Firebird requests (compiled BLR programs) have no server-side message metadata: the layout of each message is
	/// defined by the `blr_message` clause declared inside the BLR itself. RequestMessageFormat is the client-side
	/// counterpart of such a clause. Fields are appended with typed `add` methods that compute the same offsets and
	/// alignments used by the engine when it parses `blr_message`, and buildBlrMessageClause() generates the actual
	/// clause bytes, so declarations and accessors cannot get out of sync.
	///
	/// Non-nullable fields occupy only their value bytes. Nullable fields reserve an additional 16-bit null indicator
	/// right after the value, exactly like DSQL messages do.
	///
	class FB_CPP_EXPORT RequestMessageFormat final
	{
	public:
		///
		/// Appends a boolean field, optionally nullable.
		///
		RequestMessageFormat& addBool(bool isNullable)
		{
			return addField(DescriptorAdjustedType::BOOLEAN, 1u, 1u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a 16-bit signed integer field, optionally nullable.
		///
		RequestMessageFormat& addInt16(bool isNullable)
		{
			return addField(DescriptorAdjustedType::INT16, 2u, 2u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a scaled 16-bit signed integer field, optionally nullable.
		///
		RequestMessageFormat& addScaledInt16(bool isNullable, int scale)
		{
			return addField(DescriptorAdjustedType::INT16, 2u, 2u, scale, 0, 0, isNullable);
		}

		///
		/// Appends a 32-bit signed integer field, optionally nullable.
		///
		RequestMessageFormat& addInt32(bool isNullable)
		{
			return addField(DescriptorAdjustedType::INT32, 4u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a scaled 32-bit signed integer field, optionally nullable.
		///
		RequestMessageFormat& addScaledInt32(bool isNullable, int scale)
		{
			return addField(DescriptorAdjustedType::INT32, 4u, 4u, scale, 0, 0, isNullable);
		}

		///
		/// Appends a 64-bit signed integer field, optionally nullable.
		///
		RequestMessageFormat& addInt64(bool isNullable)
		{
			return addField(DescriptorAdjustedType::INT64, 8u, 8u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a scaled 64-bit signed integer field, optionally nullable.
		///
		RequestMessageFormat& addScaledInt64(bool isNullable, int scale)
		{
			return addField(DescriptorAdjustedType::INT64, 8u, 8u, scale, 0, 0, isNullable);
		}

		///
		/// Appends a scaled 128-bit integer field in Firebird's representation, optionally nullable.
		///
		RequestMessageFormat& addScaledOpaqueInt128(bool isNullable, int scale)
		{
			return addField(DescriptorAdjustedType::INT128, 16u, 8u, scale, 0, 0, isNullable);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// Appends a scaled 128-bit integer field expressed with Boost.Multiprecision, optionally nullable.
		///
		RequestMessageFormat& addScaledBoostInt128(bool isNullable, int scale)
		{
			return addField(DescriptorAdjustedType::INT128, 16u, 8u, scale, 0, 0, isNullable);
		}
#endif

		///
		/// Appends a single precision floating-point field, optionally nullable.
		///
		RequestMessageFormat& addFloat(bool isNullable)
		{
			return addField(DescriptorAdjustedType::FLOAT, 4u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a double precision floating-point field, optionally nullable.
		///
		RequestMessageFormat& addDouble(bool isNullable)
		{
			return addField(DescriptorAdjustedType::DOUBLE, 8u, 8u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a 16-digit decimal floating-point field, optionally nullable.
		///
		RequestMessageFormat& addDecFloat16(bool isNullable)
		{
			return addField(DescriptorAdjustedType::DECFLOAT16, 8u, 8u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a 34-digit decimal floating-point field, optionally nullable.
		///
		RequestMessageFormat& addDecFloat34(bool isNullable)
		{
			return addField(DescriptorAdjustedType::DECFLOAT34, 16u, 8u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a date field, optionally nullable.
		///
		RequestMessageFormat& addDate(bool isNullable)
		{
			return addField(DescriptorAdjustedType::DATE, 4u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a time-of-day field without timezone, optionally nullable.
		///
		RequestMessageFormat& addTime(bool isNullable)
		{
			return addField(DescriptorAdjustedType::TIME, 4u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a timestamp field without timezone, optionally nullable.
		///
		RequestMessageFormat& addTimestamp(bool isNullable)
		{
			return addField(DescriptorAdjustedType::TIMESTAMP, 8u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a time-of-day field with timezone, optionally nullable.
		///
		RequestMessageFormat& addTimeTz(bool isNullable)
		{
			return addField(DescriptorAdjustedType::TIME_TZ, 6u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a timestamp field with timezone, optionally nullable.
		///
		RequestMessageFormat& addTimestampTz(bool isNullable)
		{
			return addField(DescriptorAdjustedType::TIMESTAMP_TZ, 10u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a blob identifier field, optionally nullable.
		///
		RequestMessageFormat& addBlobId(bool isNullable)
		{
			return addField(DescriptorAdjustedType::BLOB, 8u, 4u, 0, 0, 0, isNullable);
		}

		///
		/// Appends a variable-length string field with the given maximum length in bytes, optionally nullable.
		/// When `charSetId` is not given, the connection character set is used.
		///
		RequestMessageFormat& addString(bool isNullable, unsigned length, std::optional<unsigned> charSetId = {})
		{
			if (length == 0u || length > 65535u - sizeof(std::uint16_t))
				throw FbCppException("Invalid string length");

			return addField(DescriptorAdjustedType::STRING, length + sizeof(std::uint16_t), sizeof(std::uint16_t), 0,
				charSetId.value_or(0u), 0, isNullable);
		}

	public:
		///
		/// Returns the number of fields.
		///
		unsigned getCount() const noexcept
		{
			return static_cast<unsigned>(descriptors.size());
		}

		///
		/// Returns the descriptors of the appended fields, in declaration order.
		///
		const std::vector<Descriptor>& getDescriptors() const noexcept
		{
			return descriptors;
		}

		///
		/// Returns the total message length in bytes.
		///
		unsigned getLength() const noexcept
		{
			return length;
		}

		///
		/// Returns the index of the field inside the BLR message, which interleaves null indicators as extra entries.
		/// Use it to write `blr_parameter` references for non-nullable fields and `blr_parameter2` references for
		/// nullable fields, whose null indicator is the next entry.
		///
		unsigned getBlrFieldIndex(unsigned index) const
		{
			if (index >= blrFieldIndexes.size())
				throw std::out_of_range("index out of range");

			return blrFieldIndexes[index];
		}

		///
		/// Generates the `blr_message` clause describing this message. Embed the returned bytes in your BLR before
		/// the body statements.
		///
		std::vector<std::byte> buildBlrMessageClause(unsigned messageNumber) const;

	private:
		RequestMessageFormat& addField(DescriptorAdjustedType adjustedType, unsigned dataLength, unsigned alignment,
			int scale, unsigned charSetId, int subType, bool isNullable);

		static void appendByte(std::vector<std::byte>& blr, std::uint8_t value)
		{
			blr.push_back(static_cast<std::byte>(value));
		}

		static void appendWord(std::vector<std::byte>& blr, std::uint16_t value)
		{
			blr.push_back(static_cast<std::byte>(value & 0xffu));
			blr.push_back(static_cast<std::byte>((value >> 8) & 0xffu));
		}

		void appendBlrDataType(std::vector<std::byte>& blr, const Descriptor& descriptor) const;

	private:
		std::vector<Descriptor> descriptors;
		std::vector<unsigned> blrFieldIndexes;
		unsigned nullableCount = 0;
		unsigned length = 0;
	};
}  // namespace fbcpp::request


#endif  // FBCPP_REQUEST_MESSAGE_FORMAT_H
