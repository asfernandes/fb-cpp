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

#ifndef FBCPP_IMPL_PARAMETER_SETTER_H
#define FBCPP_IMPL_PARAMETER_SETTER_H

#include "../config.h"
#include "../fb-api.h"
#include "../Blob.h"
#include "../CalendarConverter.h"
#include "../Client.h"
#include "../Descriptor.h"
#include "../Exception.h"
#include "../NumericConverter.h"
#include "../StructBinding.h"
#include "../types.h"
#include "../VariantTypeTraits.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
#include <boost/decimal.hpp>
#endif


namespace fbcpp::impl
{
	///
	/// Common parameter-writing API for message-backed objects.
	///
	/// The derived class supplies the input descriptors, message buffer, client, and validity check through private
	/// hooks. The boolean template argument selects the null-indicator layout used by RequestMessage.
	///
	template <typename Derived, bool RequestMessageLayout>
	class ParameterSetter
	{
	public:
		///
		/// @name Parameter writing
		/// @{

		///
		/// Marks all writable parameters as null values.
		///
		void clearParameters()
		{
			validateSetter();

			for (const auto& descriptor : descriptors())
			{
				if constexpr (RequestMessageLayout)
				{
					if (!descriptor.isNullable)
						continue;
				}

				writeNullFlag(descriptor, FB_TRUE);
			}
		}

		///
		/// Marks the specified parameter as null.
		///
		void setNull(unsigned index)
		{
			validateSetter();

			const auto& descriptor = getDescriptor(index);

			if constexpr (RequestMessageLayout)
			{
				if (!descriptor.isNullable)
					throwNonNullable(index);
			}

			writeNullFlag(descriptor, FB_TRUE);
		}

		///
		/// Binds a boolean parameter value or null.
		///
		void setBool(unsigned index, std::optional<bool> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::BOOLEAN, "bool",
				[](std::byte* data, bool value) { *data = value ? std::byte{1} : std::byte{0}; });
		}

		///
		/// Binds a 16-bit signed integer value or null.
		///
		void setInt16(unsigned index, std::optional<std::int16_t> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT16, optValue.value(), 0, "std::int16_t");
		}

		///
		/// Binds a scaled 16-bit signed integer value or null.
		///
		void setScaledInt16(unsigned index, std::optional<ScaledInt16> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT16, value.value, value.scale, "ScaledInt16");
		}

		///
		/// Binds a 32-bit signed integer value or null.
		///
		void setInt32(unsigned index, std::optional<std::int32_t> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT32, optValue.value(), 0, "std::int32_t");
		}

		///
		/// Binds a scaled 32-bit signed integer value or null.
		///
		void setScaledInt32(unsigned index, std::optional<ScaledInt32> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT32, value.value, value.scale, "ScaledInt32");
		}

		///
		/// Binds a 64-bit signed integer value or null.
		///
		void setInt64(unsigned index, std::optional<std::int64_t> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT64, optValue.value(), 0, "std::int64_t");
		}

		///
		/// Binds a scaled 64-bit signed integer value or null.
		///
		void setScaledInt64(unsigned index, std::optional<ScaledInt64> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT64, value.value, value.scale, "ScaledInt64");
		}

		///
		/// Binds a raw 128-bit integer value in Firebird's representation or null.
		///
		void setOpaqueInt128(unsigned index, std::optional<OpaqueInt128> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::INT128, "OpaqueInt128",
				[](std::byte* data, const OpaqueInt128& value) { *reinterpret_cast<OpaqueInt128*>(data) = value; });
		}

		///
		/// Binds a scaled 128-bit integer in Firebird's representation or null.
		///
		void setScaledOpaqueInt128(unsigned index, std::optional<ScaledOpaqueInt128> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			validateSetter();

			const auto& value = optValue.value();
			const auto& descriptor = getDescriptor(index);
			auto* const data = message().data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT128:
					if (value.scale == descriptor.scale)
						*reinterpret_cast<OpaqueInt128*>(&data[descriptor.offset]) = value.value;
					else
					{
						const auto valueString =
							numericConverter().opaqueInt128ToString(&statusWrapper(), value.value, value.scale);
						client()
							.getInt128Util(&statusWrapper())
							->fromString(&statusWrapper(), descriptor.scale, valueString.c_str(),
								reinterpret_cast<OpaqueInt128*>(&data[descriptor.offset]));
					}
					break;

				default:
					throwInvalidType("ScaledOpaqueInt128", descriptor.adjustedType);
			}

			writeNotNullFlag(descriptor);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// Binds a 128-bit integer value expressed with Boost.Multiprecision or null.
		///
		void setBoostInt128(unsigned index, std::optional<BoostInt128> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT128, optValue.value(), 0, "BoostInt128");
		}

		///
		/// Binds a scaled 128-bit integer value expressed with Boost.Multiprecision or null.
		///
		void setScaledBoostInt128(unsigned index, std::optional<ScaledBoostInt128> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT128, value.value, value.scale, "ScaledBoostInt128");
		}
#endif

		///
		/// Binds a single precision floating-point value or null.
		///
		void setFloat(unsigned index, std::optional<float> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::FLOAT, optValue.value(), 0, "float");
		}

		///
		/// Binds a double precision floating-point value or null.
		///
		void setDouble(unsigned index, std::optional<double> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DOUBLE, optValue.value(), 0, "double");
		}

		///
		/// Binds a 16-digit decimal floating-point value in Firebird's representation or null.
		///
		void setOpaqueDecFloat16(unsigned index, std::optional<OpaqueDecFloat16> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::DECFLOAT16, "OpaqueDecFloat16",
				[](std::byte* data, const OpaqueDecFloat16& value)
				{ *reinterpret_cast<OpaqueDecFloat16*>(data) = value; });
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// Binds a 16-digit decimal floating-point value using Boost.Multiprecision or null.
		///
		void setBoostDecFloat16(unsigned index, std::optional<BoostDecFloat16> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DECFLOAT16, optValue.value(), 0, "BoostDecFloat16");
		}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		///
		/// Binds a 7-digit decimal floating-point value using Boost.Decimal or null.
		///
		void setBoostDecimal32(unsigned index, std::optional<BoostDecimal32> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DECFLOAT16, optValue.value(), 0, "BoostDecimal32");
		}

		///
		/// Binds a 16-digit decimal floating-point value using Boost.Decimal or null.
		///
		void setBoostDecimal64(unsigned index, std::optional<BoostDecimal64> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DECFLOAT16, optValue.value(), 0, "BoostDecimal64");
		}
#endif

		///
		/// Binds a 34-digit decimal floating-point value in Firebird's representation or null.
		///
		void setOpaqueDecFloat34(unsigned index, std::optional<OpaqueDecFloat34> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::DECFLOAT34, "OpaqueDecFloat34",
				[](std::byte* data, const OpaqueDecFloat34& value)
				{ *reinterpret_cast<OpaqueDecFloat34*>(data) = value; });
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// Binds a 34-digit decimal floating-point value using Boost.Multiprecision or null.
		///
		void setBoostDecFloat34(unsigned index, std::optional<BoostDecFloat34> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DECFLOAT34, optValue.value(), 0, "BoostDecFloat34");
		}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		///
		/// Binds a 34-digit decimal floating-point value using Boost.Decimal or null.
		///
		void setBoostDecimal128(unsigned index, std::optional<BoostDecimal128> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DECFLOAT34, optValue.value(), 0, "BoostDecimal128");
		}
#endif

		///
		/// Binds a date value or null.
		///
		void setDate(unsigned index, std::optional<Date> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::DATE, "Date", [this](std::byte* data, const Date& value)
				{ *reinterpret_cast<OpaqueDate*>(data) = calendarConverter().dateToOpaqueDate(value); });
		}

		///
		/// Binds a raw date value in Firebird's representation or null.
		///
		void setOpaqueDate(unsigned index, std::optional<OpaqueDate> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::DATE, "OpaqueDate",
				[](std::byte* data, const OpaqueDate& value) { *reinterpret_cast<OpaqueDate*>(data) = value; });
		}

		///
		/// Binds a time-of-day value without timezone or null.
		///
		void setTime(unsigned index, std::optional<Time> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIME, "Time", [this](std::byte* data, const Time& value)
				{ *reinterpret_cast<OpaqueTime*>(data) = calendarConverter().timeToOpaqueTime(value); });
		}

		///
		/// Binds a raw time-of-day value in Firebird's representation or null.
		///
		void setOpaqueTime(unsigned index, std::optional<OpaqueTime> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIME, "OpaqueTime",
				[](std::byte* data, const OpaqueTime& value) { *reinterpret_cast<OpaqueTime*>(data) = value; });
		}

		///
		/// Binds a timestamp value without timezone or null.
		///
		void setTimestamp(unsigned index, std::optional<Timestamp> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIMESTAMP, "Timestamp",
				[this](std::byte* data, const Timestamp& value)
				{ *reinterpret_cast<OpaqueTimestamp*>(data) = calendarConverter().timestampToOpaqueTimestamp(value); });
		}

		///
		/// Binds a raw timestamp value in Firebird's representation or null.
		///
		void setOpaqueTimestamp(unsigned index, std::optional<OpaqueTimestamp> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIMESTAMP, "OpaqueTimestamp",
				[](std::byte* data, const OpaqueTimestamp& value)
				{ *reinterpret_cast<OpaqueTimestamp*>(data) = value; });
		}

		///
		/// Binds a time-of-day value with timezone or null.
		///
		void setTimeTz(unsigned index, std::optional<TimeTz> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIME_TZ, "TimeTz",
				[this](std::byte* data, const TimeTz& value)
				{
					*reinterpret_cast<OpaqueTimeTz*>(data) =
						calendarConverter().timeTzToOpaqueTimeTz(&statusWrapper(), value);
				});
		}

		///
		/// Binds a raw time-of-day value with timezone in Firebird's representation or null.
		///
		void setOpaqueTimeTz(unsigned index, std::optional<OpaqueTimeTz> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIME_TZ, "OpaqueTimeTz",
				[](std::byte* data, const OpaqueTimeTz& value) { *reinterpret_cast<OpaqueTimeTz*>(data) = value; });
		}

		///
		/// Binds a timestamp value with timezone or null.
		///
		void setTimestampTz(unsigned index, std::optional<TimestampTz> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIMESTAMP_TZ, "TimestampTz",
				[this](std::byte* data, const TimestampTz& value)
				{
					*reinterpret_cast<OpaqueTimestampTz*>(data) =
						calendarConverter().timestampTzToOpaqueTimestampTz(&statusWrapper(), value);
				});
		}

		///
		/// Binds a raw timestamp value with timezone in Firebird's representation or null.
		///
		void setOpaqueTimestampTz(unsigned index, std::optional<OpaqueTimestampTz> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::TIMESTAMP_TZ, "OpaqueTimestampTz",
				[](std::byte* data, const OpaqueTimestampTz& value)
				{ *reinterpret_cast<OpaqueTimestampTz*>(data) = value; });
		}

		///
		/// Binds a textual parameter or null, performing direct conversions where supported.
		///
		void setString(unsigned index, std::optional<std::string_view> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			validateSetter();

			auto& clientValue = client();
			const auto value = optValue.value();
			const auto& descriptor = getDescriptor(index);
			auto* const data = message().data();
			const auto descriptorData = &data[descriptor.offset];

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					data[descriptor.offset] = numericConverter().stringToBoolean(value);
					break;

				case DescriptorAdjustedType::INT16:
				case DescriptorAdjustedType::INT32:
				case DescriptorAdjustedType::INT64:
				{
					std::string strValue(value);
					int scale = 0;

					if (const auto dotPos = strValue.find_last_of('.'); dotPos != std::string_view::npos)
					{
						for (auto pos = dotPos + 1; pos < strValue.size(); ++pos)
						{
							const char c = value[pos];

							if (c < '0' || c > '9')
								break;

							--scale;
						}

						strValue.erase(dotPos, 1);
					}

					static_assert(sizeof(long long) == sizeof(std::int64_t));
					std::int64_t intValue;
					const auto convResult =
						std::from_chars(strValue.data(), strValue.data() + strValue.size(), intValue);
					if (convResult.ec != std::errc{} || convResult.ptr != strValue.data() + strValue.size())
						numericConverter().throwConversionErrorFromString(strValue);
					auto scaledValue = ScaledInt64{intValue, scale};

					if (scale != descriptor.scale)
					{
						scaledValue.value =
							numericConverter().template numberToNumber<std::int64_t>(scaledValue, descriptor.scale);
						scaledValue.scale = descriptor.scale;
					}

					setScaledInt64(index, scaledValue);
					return;
				}

				case DescriptorAdjustedType::INT128:
				{
					std::string strValue(value);
					clientValue.getInt128Util(&statusWrapper())
						->fromString(&statusWrapper(), descriptor.scale, strValue.c_str(),
							reinterpret_cast<OpaqueInt128*>(descriptorData));
					break;
				}

				case DescriptorAdjustedType::FLOAT:
				case DescriptorAdjustedType::DOUBLE:
				{
					double doubleValue;
#if defined(__APPLE__)
					errno = 0;
					std::string valueString{value};
					char* parseEnd = nullptr;
					doubleValue = std::strtod(valueString.c_str(), &parseEnd);
					if (parseEnd != valueString.c_str() + valueString.size() || errno == ERANGE)
						numericConverter().throwConversionErrorFromString(std::move(valueString));
#else
					const auto convResult = std::from_chars(value.data(), value.data() + value.size(), doubleValue);
					if (convResult.ec != std::errc{} || convResult.ptr != value.data() + value.size())
						numericConverter().throwConversionErrorFromString(std::string{value});
#endif
					setDouble(index, doubleValue);
					return;
				}

				case DescriptorAdjustedType::DATE:
					*reinterpret_cast<OpaqueDate*>(descriptorData) = calendarConverter().stringToOpaqueDate(value);
					break;

				case DescriptorAdjustedType::TIME:
					*reinterpret_cast<OpaqueTime*>(descriptorData) = calendarConverter().stringToOpaqueTime(value);
					break;

				case DescriptorAdjustedType::TIMESTAMP:
					*reinterpret_cast<OpaqueTimestamp*>(descriptorData) =
						calendarConverter().stringToOpaqueTimestamp(value);
					break;

				case DescriptorAdjustedType::TIME_TZ:
					*reinterpret_cast<OpaqueTimeTz*>(descriptorData) =
						calendarConverter().stringToOpaqueTimeTz(&statusWrapper(), value);
					break;

				case DescriptorAdjustedType::TIMESTAMP_TZ:
					*reinterpret_cast<OpaqueTimestampTz*>(descriptorData) =
						calendarConverter().stringToOpaqueTimestampTz(&statusWrapper(), value);
					break;

				case DescriptorAdjustedType::DECFLOAT16:
				{
					std::string strValue{value};
					clientValue.getDecFloat16Util(&statusWrapper())
						->fromString(
							&statusWrapper(), strValue.c_str(), reinterpret_cast<OpaqueDecFloat16*>(descriptorData));
					break;
				}

				case DescriptorAdjustedType::DECFLOAT34:
				{
					std::string strValue{value};
					clientValue.getDecFloat34Util(&statusWrapper())
						->fromString(
							&statusWrapper(), strValue.c_str(), reinterpret_cast<OpaqueDecFloat34*>(descriptorData));
					break;
				}

				case DescriptorAdjustedType::STRING:
					if (value.length() > stringCapacity(descriptor))
					{
						static constexpr std::intptr_t STATUS_STRING_TRUNCATION[] = {
							isc_arith_except,
							isc_string_truncation,
							isc_arg_end,
						};

						throw DatabaseException(clientValue, STATUS_STRING_TRUNCATION);
					}

					*reinterpret_cast<std::uint16_t*>(descriptorData) = static_cast<std::uint16_t>(value.length());
					std::copy(value.begin(), value.end(),
						reinterpret_cast<char*>(&data[descriptor.offset + sizeof(std::uint16_t)]));
					break;

				default:
					throwInvalidType("std::string_view", descriptor.adjustedType);
			}

			writeNotNullFlag(descriptor);
		}

		///
		/// Binds raw bytes to a text or varying parameter.
		///
		void setBytes(unsigned index, std::span<const std::byte> value)
		{
			validateSetter();

			const auto& descriptor = getDescriptor(index);
			auto* const data = message().data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::STRING:
					if (value.size() > stringCapacity(descriptor))
					{
						static constexpr std::intptr_t STATUS_STRING_TRUNCATION[] = {
							isc_arith_except,
							isc_string_truncation,
							isc_arg_end,
						};

						throw DatabaseException(client(), STATUS_STRING_TRUNCATION);
					}

					*reinterpret_cast<std::uint16_t*>(&data[descriptor.offset]) =
						static_cast<std::uint16_t>(value.size());
					std::copy(value.begin(), value.end(), &data[descriptor.offset + sizeof(std::uint16_t)]);
					break;

				default:
					throwInvalidType("std::span<const std::byte>", descriptor.adjustedType);
			}

			writeNotNullFlag(descriptor);
		}

		///
		/// Binds a vector of raw bytes to a text or varying parameter.
		///
		void setBytes(unsigned index, const std::vector<std::byte>& value)
		{
			setBytes(index, std::span<const std::byte>{value});
		}

		///
		/// Binds an optional vector of raw bytes to a text or varying parameter.
		///
		void setBytes(unsigned index, std::optional<std::vector<std::byte>> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setBytes(index, std::span<const std::byte>{optValue.value()});
		}

		///
		/// Binds a null byte value.
		///
		void setBytes(unsigned index, std::nullopt_t)
		{
			setNull(index);
		}

		///
		/// Binds a blob identifier to the specified parameter or null.
		///
		void setBlobId(unsigned index, std::optional<BlobId> optValue)
		{
			setExact(index, optValue, DescriptorAdjustedType::BLOB, "BlobId",
				[](std::byte* data, const BlobId& value) { *reinterpret_cast<ISC_QUAD*>(data) = value.id; });
		}

		///
		/// @name Convenience overloads
		/// @{

		///
		/// Convenience overload that binds a null value.
		///
		void set(unsigned index, std::nullopt_t)
		{
			setNull(index);
		}

		///
		/// Convenience overload that binds a blob identifier.
		///
		void set(unsigned index, BlobId value)
		{
			setBlobId(index, value);
		}

		///
		/// Convenience overload that binds an optional blob identifier.
		///
		void set(unsigned index, std::optional<BlobId> value)
		{
			setBlobId(index, value);
		}

		///
		/// Convenience overload that binds a boolean value.
		///
		void set(unsigned index, bool value)
		{
			setBool(index, value);
		}

		///
		/// Convenience overload that binds a 16-bit signed integer.
		///
		void set(unsigned index, std::int16_t value)
		{
			setInt16(index, value);
		}

		///
		/// Convenience overload that binds a scaled 16-bit signed integer.
		///
		void set(unsigned index, ScaledInt16 value)
		{
			setScaledInt16(index, value);
		}

		///
		/// Convenience overload that binds a 32-bit signed integer.
		///
		void set(unsigned index, std::int32_t value)
		{
			setInt32(index, value);
		}

		///
		/// Convenience overload that binds a scaled 32-bit signed integer.
		///
		void set(unsigned index, ScaledInt32 value)
		{
			setScaledInt32(index, value);
		}

		///
		/// Convenience overload that binds a 64-bit signed integer.
		///
		void set(unsigned index, std::int64_t value)
		{
			setInt64(index, value);
		}

		///
		/// Convenience overload that binds a scaled 64-bit signed integer.
		///
		void set(unsigned index, ScaledInt64 value)
		{
			setScaledInt64(index, value);
		}

		///
		/// Convenience overload that binds a Firebird 128-bit integer.
		///
		void set(unsigned index, OpaqueInt128 value)
		{
			setOpaqueInt128(index, value);
		}

		///
		/// Convenience overload that binds a scaled Firebird 128-bit integer.
		///
		void set(unsigned index, ScaledOpaqueInt128 value)
		{
			setScaledOpaqueInt128(index, value);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// Convenience overload that binds a Boost-provided 128-bit integer.
		///
		void set(unsigned index, BoostInt128 value)
		{
			setBoostInt128(index, value);
		}

		///
		/// Convenience overload that binds a scaled Boost-provided 128-bit integer.
		///
		void set(unsigned index, ScaledBoostInt128 value)
		{
			setScaledBoostInt128(index, value);
		}
#endif

		///
		/// Convenience overload that binds a single precision floating-point value.
		///
		void set(unsigned index, float value)
		{
			setFloat(index, value);
		}

		///
		/// Convenience overload that binds a double precision floating-point value.
		///
		void set(unsigned index, double value)
		{
			setDouble(index, value);
		}

		///
		/// Convenience overload that binds a Firebird 16-digit decimal floating-point value.
		///
		void set(unsigned index, OpaqueDecFloat16 value)
		{
			setOpaqueDecFloat16(index, value);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// Convenience overload that binds a Boost 16-digit decimal floating-point value.
		///
		void set(unsigned index, BoostDecFloat16 value)
		{
			setBoostDecFloat16(index, value);
		}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		///
		/// Convenience overload that binds a Boost.Decimal 7-digit decimal floating-point value.
		///
		void set(unsigned index, BoostDecimal32 value)
		{
			setBoostDecimal32(index, value);
		}

		///
		/// Convenience overload that binds a Boost.Decimal 16-digit decimal floating-point value.
		///
		void set(unsigned index, BoostDecimal64 value)
		{
			setBoostDecimal64(index, value);
		}
#endif

		///
		/// Convenience overload that binds a Firebird 34-digit decimal floating-point value.
		///
		void set(unsigned index, OpaqueDecFloat34 value)
		{
			setOpaqueDecFloat34(index, value);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// Convenience overload that binds a Boost 34-digit decimal floating-point value.
		///
		void set(unsigned index, BoostDecFloat34 value)
		{
			setBoostDecFloat34(index, value);
		}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		///
		/// Convenience overload that binds a Boost.Decimal 34-digit decimal floating-point value.
		///
		void set(unsigned index, BoostDecimal128 value)
		{
			setBoostDecimal128(index, value);
		}
#endif

		///
		/// Convenience overload that binds a Firebird date value.
		///
		void set(unsigned index, Date value)
		{
			setDate(index, value);
		}

		///
		/// Convenience overload that binds a raw Firebird date value.
		///
		void set(unsigned index, OpaqueDate value)
		{
			setOpaqueDate(index, value);
		}

		///
		/// Convenience overload that binds a Firebird time value.
		///
		void set(unsigned index, Time value)
		{
			setTime(index, value);
		}

		///
		/// Convenience overload that binds a raw Firebird time value.
		///
		void set(unsigned index, OpaqueTime value)
		{
			setOpaqueTime(index, value);
		}

		///
		/// Convenience overload that binds a Firebird timestamp value.
		///
		void set(unsigned index, Timestamp value)
		{
			setTimestamp(index, value);
		}

		///
		/// Convenience overload that binds a raw Firebird timestamp value.
		///
		void set(unsigned index, OpaqueTimestamp value)
		{
			setOpaqueTimestamp(index, value);
		}

		///
		/// Convenience overload that binds a Firebird time with timezone value.
		///
		void set(unsigned index, TimeTz value)
		{
			setTimeTz(index, value);
		}

		///
		/// Convenience overload that binds a raw Firebird time with timezone value.
		///
		void set(unsigned index, OpaqueTimeTz value)
		{
			setOpaqueTimeTz(index, value);
		}

		///
		/// Convenience overload that binds a Firebird timestamp with timezone value.
		///
		void set(unsigned index, TimestampTz value)
		{
			setTimestampTz(index, value);
		}

		///
		/// Convenience overload that binds a raw Firebird timestamp with timezone value.
		///
		void set(unsigned index, OpaqueTimestampTz value)
		{
			setOpaqueTimestampTz(index, value);
		}

		///
		/// Convenience overload that binds a textual value.
		///
		void set(unsigned index, std::string_view value)
		{
			setString(index, value);
		}

		///
		/// Convenience overload that binds a vector of raw bytes.
		///
		void set(unsigned index, const std::vector<std::byte>& value)
		{
			setBytes(index, value);
		}

		///
		/// Convenience overload that binds a span of raw bytes.
		///
		void set(unsigned index, std::span<const std::byte> value)
		{
			setBytes(index, value);
		}

		///
		/// Convenience template that forwards optional values to specialized overloads.
		///
		template <typename T>
		void set(unsigned index, std::optional<T> value)
		{
			if (value.has_value())
				set(index, value.value());
			else
				setNull(index);
		}

		///
		/// @}
		///

		///
		/// Sets all input parameters from fields of a user-defined aggregate struct.
		///
		template <Aggregate T>
		void set(const T& value)
		{
			using namespace reflection;

			constexpr std::size_t N = fieldCountV<T>;

			if (N != descriptors().size())
			{
				throw FbCppException("Struct field count (" + std::to_string(N) +
					") does not match input parameter count (" + std::to_string(descriptors().size()) + ")");
			}

			setStruct(value, std::make_index_sequence<N>{});
		}

		///
		/// Sets all input parameters from elements of a tuple-like type.
		///
		template <TupleLike T>
		void set(const T& value)
		{
			constexpr std::size_t N = std::tuple_size_v<T>;

			if (N != descriptors().size())
			{
				throw FbCppException("Tuple element count (" + std::to_string(N) +
					") does not match input parameter count (" + std::to_string(descriptors().size()) + ")");
			}

			setTuple(value, std::make_index_sequence<N>{});
		}

		///
		/// Sets a parameter from a variant value.
		///
		template <VariantLike V>
		void set(unsigned index, const V& value)
		{
			using namespace reflection;

			static_assert(variantAlternativesSupportedV<V>,
				"Variant contains unsupported types. All variant alternatives must be types supported by fb-cpp "
				"(e.g., std::int32_t, std::string, Date, ScaledOpaqueInt128, etc.). Check VariantTypeTraits.h for "
				"the complete list of supported types.");

			std::visit(
				[this, index](const auto& v)
				{
					using T = std::decay_t<decltype(v)>;

					if constexpr (std::is_same_v<T, std::monostate>)
						setNull(index);
					else
						set(index, v);
				},
				value);
		}

		///
		/// @}
		///

	private:
		Derived& owner() noexcept
		{
			return static_cast<Derived&>(*this);
		}

		const Derived& owner() const noexcept
		{
			return static_cast<const Derived&>(*this);
		}

		const std::vector<Descriptor>& descriptors() const noexcept
		{
			return owner().getSetterDescriptors();
		}

		std::vector<std::byte>& message() noexcept
		{
			return owner().getSetterMessage();
		}

		Client& client() noexcept
		{
			return owner().getSetterClient();
		}

		impl::StatusWrapper& statusWrapper() noexcept
		{
			return owner().statusWrapper;
		}

		impl::NumericConverter& numericConverter() noexcept
		{
			return owner().numericConverter;
		}

		impl::CalendarConverter& calendarConverter() noexcept
		{
			return owner().calendarConverter;
		}

		void validateSetter() const noexcept
		{
			owner().validateSetter();
		}

		const Descriptor& getDescriptor(unsigned index) const
		{
			if (index >= descriptors().size())
				throw std::out_of_range("index out of range");

			return descriptors()[index];
		}

		template <typename T, typename Writer>
		void setExact(unsigned index, std::optional<T> optValue, DescriptorAdjustedType expectedType,
			const char* typeName, Writer writer)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			validateSetter();

			const auto& descriptor = getDescriptor(index);

			if (descriptor.adjustedType != expectedType)
				throwInvalidType(typeName, descriptor.adjustedType);

			writer(&message()[descriptor.offset], optValue.value());
			writeNotNullFlag(descriptor);
		}

#if FB_CPP_USE_BOOST_DECIMAL != 0
		template <typename T>
		void setDecimalNumber(unsigned index, T value, std::optional<int>& descriptorScale, const char* typeName)
		{
			const auto& descriptor = getDescriptor(index);
			auto* const data = message().data();
			const auto descriptorData = &data[descriptor.offset];

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT16:
					*reinterpret_cast<std::int16_t*>(descriptorData) =
						numericConverter().template numberToNumber<std::int16_t>(value, descriptorScale.value());
					break;

				case DescriptorAdjustedType::INT32:
					*reinterpret_cast<std::int32_t*>(descriptorData) =
						numericConverter().template numberToNumber<std::int32_t>(value, descriptorScale.value());
					break;

				case DescriptorAdjustedType::INT64:
					*reinterpret_cast<std::int64_t*>(descriptorData) =
						numericConverter().template numberToNumber<std::int64_t>(value, descriptorScale.value());
					break;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
				{
					const auto int128Value =
						numericConverter().template numberToNumber<BoostInt128>(value, descriptorScale.value());
					*reinterpret_cast<OpaqueInt128*>(descriptorData) =
						numericConverter().boostInt128ToOpaqueInt128(&statusWrapper(), int128Value);
					break;
				}
#endif

				case DescriptorAdjustedType::FLOAT:
					*reinterpret_cast<float*>(descriptorData) =
						numericConverter().template numberToNumber<float>(value);
					break;

				case DescriptorAdjustedType::DOUBLE:
					*reinterpret_cast<double*>(descriptorData) =
						numericConverter().template numberToNumber<double>(value);
					break;

				case DescriptorAdjustedType::DECFLOAT16:
				{
					const auto decimalValue = numericConverter().template numberToNumber<BoostDecimal64>(value);
					*reinterpret_cast<OpaqueDecFloat16*>(descriptorData) =
						numericConverter().boostDecimal64ToOpaqueDecFloat16(&statusWrapper(), decimalValue);
					break;
				}

				case DescriptorAdjustedType::DECFLOAT34:
				{
					const auto decimalValue = numericConverter().template numberToNumber<BoostDecimal128>(value);
					*reinterpret_cast<OpaqueDecFloat34*>(descriptorData) =
						numericConverter().boostDecimal128ToOpaqueDecFloat34(&statusWrapper(), decimalValue);
					break;
				}

				default:
					throwInvalidType(typeName, descriptor.adjustedType);
			}
		}
#endif

		template <typename T>
		void setNumber(unsigned index, DescriptorAdjustedType valueType, T value, int scale, const char* typeName)
		{
			validateSetter();

			const auto& descriptor = getDescriptor(index);
			auto* const data = message().data();
			const auto descriptorData = &data[descriptor.offset];
			std::optional<int> descriptorScale{descriptor.scale};

#if FB_CPP_USE_BOOST_DECIMAL != 0
			if constexpr (std::is_same_v<T, BoostDecimal32> || std::is_same_v<T, BoostDecimal64> ||
				std::is_same_v<T, BoostDecimal128>)
			{
				setDecimalNumber(index, value, descriptorScale, typeName);
				writeNotNullFlag(descriptor);
				return;
			}
#endif

			Descriptor valueDescriptor;
			valueDescriptor.adjustedType = valueType;
			valueDescriptor.scale = scale;

			const auto valueAddress = reinterpret_cast<const std::byte*>(&value);

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT16:
					*reinterpret_cast<std::int16_t*>(descriptorData) =
						convertNumber<std::int16_t>(valueDescriptor, valueAddress, descriptorScale, "std::int16_t");
					break;

				case DescriptorAdjustedType::INT32:
					*reinterpret_cast<std::int32_t*>(descriptorData) =
						convertNumber<std::int32_t>(valueDescriptor, valueAddress, descriptorScale, "std::int32_t");
					break;

				case DescriptorAdjustedType::INT64:
					*reinterpret_cast<std::int64_t*>(descriptorData) =
						convertNumber<std::int64_t>(valueDescriptor, valueAddress, descriptorScale, "std::int64_t");
					break;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
				{
					const auto boostInt128 =
						convertNumber<BoostInt128>(valueDescriptor, valueAddress, descriptorScale, "BoostInt128");
					*reinterpret_cast<OpaqueInt128*>(descriptorData) =
						numericConverter().boostInt128ToOpaqueInt128(&statusWrapper(), boostInt128);
					break;
				}
#endif

				case DescriptorAdjustedType::FLOAT:
					*reinterpret_cast<float*>(descriptorData) =
						convertNumber<float>(valueDescriptor, valueAddress, descriptorScale, "float");
					break;

				case DescriptorAdjustedType::DOUBLE:
					*reinterpret_cast<double*>(descriptorData) =
						convertNumber<double>(valueDescriptor, valueAddress, descriptorScale, "double");
					break;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::DECFLOAT16:
				{
					const auto boostDecFloat16 = convertNumber<BoostDecFloat16>(
						valueDescriptor, valueAddress, descriptorScale, "BoostDecFloat16");
					*reinterpret_cast<OpaqueDecFloat16*>(descriptorData) =
						numericConverter().boostDecFloat16ToOpaqueDecFloat16(&statusWrapper(), boostDecFloat16);
					break;
				}

				case DescriptorAdjustedType::DECFLOAT34:
				{
					const auto boostDecFloat34 = convertNumber<BoostDecFloat34>(
						valueDescriptor, valueAddress, descriptorScale, "BoostDecFloat34");
					*reinterpret_cast<OpaqueDecFloat34*>(descriptorData) =
						numericConverter().boostDecFloat34ToOpaqueDecFloat34(&statusWrapper(), boostDecFloat34);
					break;
				}
#endif

				default:
					throwInvalidType(typeName, descriptor.adjustedType);
			}

			writeNotNullFlag(descriptor);
		}

		[[noreturn]] static void throwInvalidType(const char* actualType, DescriptorAdjustedType descriptorType)
		{
			throw FbCppException("Invalid type: actual type " + std::string(actualType) + ", descriptor type " +
				std::to_string(static_cast<unsigned>(descriptorType)));
		}

		[[noreturn]] static void throwNonNullable(unsigned index)
		{
			throw FbCppException("Field " + std::to_string(index) + " is not nullable");
		}

		void writeNullFlag(const Descriptor& descriptor, std::int16_t value)
		{
			if constexpr (!RequestMessageLayout)
			{
				*reinterpret_cast<std::int16_t*>(&message()[descriptor.nullOffset]) = value;
			}
			else if (descriptor.isNullable)
				*reinterpret_cast<std::int16_t*>(&message()[descriptor.nullOffset]) = value;
		}

		void writeNotNullFlag(const Descriptor& descriptor)
		{
			writeNullFlag(descriptor, FB_FALSE);
		}

		unsigned stringCapacity(const Descriptor& descriptor) const noexcept
		{
			if constexpr (RequestMessageLayout)
				return descriptor.length - sizeof(std::uint16_t);
			else
				return descriptor.length;
		}

		template <typename T, std::size_t... Is>
		void setStruct(const T& value, std::index_sequence<Is...>)
		{
			using namespace reflection;

			const auto tuple = toTupleRef(value);
			(set(static_cast<unsigned>(Is), std::get<Is>(tuple)), ...);
		}

		template <typename T, std::size_t... Is>
		void setTuple(const T& value, std::index_sequence<Is...>)
		{
			(set(static_cast<unsigned>(Is), std::get<Is>(value)), ...);
		}

		template <typename T>
		T convertNumber(
			const Descriptor& descriptor, const std::byte* data, std::optional<int>& toScale, const char* toTypeName)
		{
			if (!toScale.has_value())
			{
				switch (descriptor.adjustedType)
				{
					case DescriptorAdjustedType::DECFLOAT16:
					case DescriptorAdjustedType::DECFLOAT34:
					case DescriptorAdjustedType::FLOAT:
					case DescriptorAdjustedType::DOUBLE:
						throwInvalidType(toTypeName, descriptor.adjustedType);

					default:
						break;
				}

				toScale = descriptor.scale;
			}

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT16:
					return numericConverter().template numberToNumber<T>(
						ScaledInt16{*reinterpret_cast<const std::int16_t*>(data), descriptor.scale}, toScale.value());

				case DescriptorAdjustedType::INT32:
					return numericConverter().template numberToNumber<T>(
						ScaledInt32{*reinterpret_cast<const std::int32_t*>(data), descriptor.scale}, toScale.value());

				case DescriptorAdjustedType::INT64:
					return numericConverter().template numberToNumber<T>(
						ScaledInt64{*reinterpret_cast<const std::int64_t*>(data), descriptor.scale}, toScale.value());

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
					return numericConverter().template numberToNumber<T>(
						ScaledBoostInt128{*reinterpret_cast<const BoostInt128*>(data), descriptor.scale},
						toScale.value());

				case DescriptorAdjustedType::DECFLOAT16:
					return numericConverter().template numberToNumber<T>(
						*reinterpret_cast<const BoostDecFloat16*>(data), toScale.value());

				case DescriptorAdjustedType::DECFLOAT34:
					return numericConverter().template numberToNumber<T>(
						*reinterpret_cast<const BoostDecFloat34*>(data), toScale.value());
#endif

				case DescriptorAdjustedType::FLOAT:
					return numericConverter().template numberToNumber<T>(
						*reinterpret_cast<const float*>(data), toScale.value());

				case DescriptorAdjustedType::DOUBLE:
					return numericConverter().template numberToNumber<T>(
						*reinterpret_cast<const double*>(data), toScale.value());

				default:
					throwInvalidType(toTypeName, descriptor.adjustedType);
			}
		}
	};
}  // namespace fbcpp::impl


#endif  // FBCPP_IMPL_PARAMETER_SETTER_H
