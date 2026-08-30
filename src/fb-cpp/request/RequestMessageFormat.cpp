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

#include "RequestMessageFormat.h"
#include "firebird/impl/blr.h"


namespace fbcpp::request
{
	static std::uint8_t scaleToByte(int scale) noexcept
	{
		return static_cast<std::uint8_t>(static_cast<unsigned>(scale) & 0xFFu);
	}


	RequestMessageFormat& RequestMessageFormat::addField(DescriptorAdjustedType adjustedType, unsigned dataLength,
		unsigned alignment, int scale, unsigned charSetId, int subType, bool isNullable)
	{
		const auto alignedOffset = static_cast<unsigned>((length + alignment - 1u) & ~(alignment - 1u));

		Descriptor descriptor{
			.originalType = static_cast<DescriptorOriginalType>(adjustedType),
			.adjustedType = adjustedType,
			.scale = scale,
			.length = dataLength,
			.offset = alignedOffset,
			.nullOffset = 0,
			.isNullable = isNullable,
			.name = {},
			.relation = {},
			.alias = {},
			.owner = {},
			.charSetId = charSetId,
			.subType = subType,
		};

		length = alignedOffset + dataLength;
		blrFieldIndexes.push_back(static_cast<unsigned>(descriptors.size()) + nullableCount);

		if (isNullable)
		{
			descriptor.nullOffset = (length + 1u) & ~1u;
			length = descriptor.nullOffset + 2u;
			++nullableCount;
		}

		descriptors.push_back(std::move(descriptor));
		return *this;
	}

	std::vector<std::byte> RequestMessageFormat::buildBlrMessageClause(unsigned messageNumber) const
	{
		if (messageNumber > 255u)
			throw FbCppException("Message number must be between 0 and 255");

		unsigned entryCount = static_cast<unsigned>(descriptors.size());
		for (const auto& descriptor : descriptors)
		{
			if (descriptor.isNullable)
				++entryCount;
		}

		std::vector<std::byte> blr;
		blr.reserve(3u + 64u * entryCount);

		appendByte(blr, blr_message);
		appendByte(blr, static_cast<std::uint8_t>(messageNumber));
		appendWord(blr, static_cast<std::uint16_t>(entryCount));

		for (const auto& descriptor : descriptors)
		{
			appendBlrDataType(blr, descriptor);

			if (descriptor.isNullable)
			{
				appendByte(blr, blr_short);
				appendByte(blr, 0u);
			}
		}

		return blr;
	}

	void RequestMessageFormat::appendBlrDataType(std::vector<std::byte>& blr, const Descriptor& descriptor) const
	{
		switch (descriptor.adjustedType)
		{
			case DescriptorAdjustedType::BOOLEAN:
				appendByte(blr, blr_bool);
				break;

			case DescriptorAdjustedType::INT16:
				appendByte(blr, blr_short);
				appendByte(blr, scaleToByte(descriptor.scale));
				break;

			case DescriptorAdjustedType::INT32:
				appendByte(blr, blr_long);
				appendByte(blr, scaleToByte(descriptor.scale));
				break;

			case DescriptorAdjustedType::INT64:
				appendByte(blr, blr_int64);
				appendByte(blr, scaleToByte(descriptor.scale));
				break;

			case DescriptorAdjustedType::INT128:
				appendByte(blr, blr_int128);
				appendByte(blr, scaleToByte(descriptor.scale));
				break;

			case DescriptorAdjustedType::FLOAT:
				appendByte(blr, blr_float);
				break;

			case DescriptorAdjustedType::DOUBLE:
				appendByte(blr, blr_double);
				break;

			case DescriptorAdjustedType::DECFLOAT16:
				appendByte(blr, blr_dec64);
				break;

			case DescriptorAdjustedType::DECFLOAT34:
				appendByte(blr, blr_dec128);
				break;

			case DescriptorAdjustedType::DATE:
				appendByte(blr, blr_sql_date);
				break;

			case DescriptorAdjustedType::TIME:
				appendByte(blr, blr_sql_time);
				break;

			case DescriptorAdjustedType::TIMESTAMP:
				appendByte(blr, blr_timestamp);
				break;

			case DescriptorAdjustedType::TIME_TZ:
				appendByte(blr, blr_sql_time_tz);
				break;

			case DescriptorAdjustedType::TIMESTAMP_TZ:
				appendByte(blr, blr_timestamp_tz);
				break;

			case DescriptorAdjustedType::BLOB:
				appendByte(blr, blr_quad);
				appendByte(blr, 0u);
				break;

			case DescriptorAdjustedType::STRING:
				if (descriptor.charSetId != 0)
				{
					appendByte(blr, blr_varying2);
					appendWord(blr, static_cast<std::uint16_t>(descriptor.charSetId));
				}
				else
					appendByte(blr, blr_varying);

				appendWord(blr, static_cast<std::uint16_t>(descriptor.length - sizeof(std::uint16_t)));
				break;

			default:
				throw FbCppException("Unsupported descriptor type");
		}
	}
}  // namespace fbcpp::request
