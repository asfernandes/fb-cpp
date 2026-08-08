/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FBCPP_NUMERIC_CONVERTER_H
#define FBCPP_NUMERIC_CONVERTER_H

#include "config.h"
#include "fb-api.h"
#include "Client.h"
#include "Exception.h"
#include "types.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


namespace fbcpp::impl
{
	template <typename T>
	struct NumberTypePriority
	{
		static constexpr int value = 0;
	};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	struct NumberTypePriority<BoostDecFloat34>
	{
		static constexpr int value = 7;
	};

	template <>
	struct NumberTypePriority<BoostDecFloat16>
	{
		static constexpr int value = 6;
	};
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
	template <>
	struct NumberTypePriority<BoostDecimal128>
	{
		static constexpr int value = 10;
	};

	template <>
	struct NumberTypePriority<BoostDecimal64>
	{
		static constexpr int value = 9;
	};

	template <>
	struct NumberTypePriority<BoostDecimal32>
	{
		static constexpr int value = 8;
	};
#endif

	template <>
	struct NumberTypePriority<double>
	{
		static constexpr int value = 6;
	};

	template <>
	struct NumberTypePriority<float>
	{
		static constexpr int value = 5;
	};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	struct NumberTypePriority<BoostInt128>
	{
		static constexpr int value = 4;
	};
#endif

	template <>
	struct NumberTypePriority<std::int64_t>
	{
		static constexpr int value = 3;
	};

	template <>
	struct NumberTypePriority<std::int32_t>
	{
		static constexpr int value = 2;
	};

	template <>
	struct NumberTypePriority<std::int16_t>
	{
		static constexpr int value = 1;
	};

	template <typename T>
	inline constexpr int NumberTypePriorityValue =
		NumberTypePriority<std::remove_cv_t<std::remove_reference_t<T>>>::value;

	template <typename T1, typename T2>
	using GreaterNumberType = std::conditional_t<(NumberTypePriorityValue<T1> >= NumberTypePriorityValue<T2>), T1, T2>;

	template <typename T>
	inline constexpr bool IsFloatingNumber = std::is_floating_point_v<T>
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		|| std::same_as<T, BoostDecFloat16> || std::same_as<T, BoostDecFloat34>
#endif
#if FB_CPP_USE_BOOST_DECIMAL != 0
		|| std::same_as<T, BoostDecimal32> || std::same_as<T, BoostDecimal64> || std::same_as<T, BoostDecimal128>
#endif
		;

	template <typename T>
	concept FloatingNumber = IsFloatingNumber<T>;

	template <typename T>
	concept IntegralNumber = std::is_integral_v<T>
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		|| std::same_as<T, BoostInt128>
#endif
		;

	template <typename T>
	struct MakeUnsigned
	{
		using type = std::make_unsigned_t<T>;
	};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	struct MakeUnsigned<BoostInt128>
	{
		using type = boost::multiprecision::uint128_t;
	};
#endif

	template <typename T>
	using MakeUnsignedType = typename MakeUnsigned<T>::type;

	class NumericConverter final
	{
	public:
		explicit NumericConverter(Client& client)
			: client{&client}
		{
		}

		[[noreturn]] void throwNumericOutOfRange()
		{
			static constexpr std::intptr_t STATUS_NUMERIC_OUT_OF_RANGE[] = {
				isc_arg_gds,
				isc_arith_except,
				isc_arg_gds,
				isc_numeric_out_of_range,
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_NUMERIC_OUT_OF_RANGE);
		}

		[[noreturn]] void throwConversionErrorFromString(const std::string& str)
		{
			const std::intptr_t STATUS_CONVERSION_ERROR_FROM_STRING[] = {
				isc_arg_gds,
				isc_convert_error,
				isc_arg_string,
				reinterpret_cast<std::intptr_t>(str.c_str()),
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_CONVERSION_ERROR_FROM_STRING);
		}

	public:
		template <IntegralNumber To, IntegralNumber From>
		To numberToNumber(const ScaledNumber<From>& from, int toScale)
		{
			const int scaleDiff = toScale - from.scale;

			using ComputeType = GreaterNumberType<decltype(from.value), To>;

			ComputeType result = static_cast<ComputeType>(from.value);

			if (scaleDiff != 0)
			{
				adjustScale(result, scaleDiff, static_cast<ComputeType>(std::numeric_limits<To>::min()),
					static_cast<ComputeType>(std::numeric_limits<To>::max()));
			}

			if (result < static_cast<ComputeType>(std::numeric_limits<To>::min()) ||
				result > static_cast<ComputeType>(std::numeric_limits<To>::max()))
			{
				throwNumericOutOfRange();
			}

			return static_cast<To>(result);
		}

		template <IntegralNumber To, FloatingNumber From>
		To numberToNumber(const From& from, int toScale)
		{
			using ComputeType = GreaterNumberType<double, From>;

			if (isNonFinite(from))
				throwNumericOutOfRange();

			ComputeType value = convertFloatingValue<ComputeType>(from);
			const ComputeType eps = conversionEpsilon<ComputeType>();
			const ComputeType half{0.5};

			if (toScale > 0)
				value /= powerOfTen<ComputeType>(toScale);
			else if (toScale < 0)
				value *= powerOfTen<ComputeType>(-toScale);

			if (value > 0)
				value += half + eps;
			else
				value -= half + eps;

			static const auto minLimit = convertFloatingValue<ComputeType>(std::numeric_limits<To>::min());
			static const auto maxLimit = convertFloatingValue<ComputeType>(std::numeric_limits<To>::max());

			if (value < minLimit)
			{
				if (value > minLimit - ComputeType{1})
					return std::numeric_limits<To>::min();
				throwNumericOutOfRange();
			}

			if (value > maxLimit)
			{
				if (value < maxLimit + ComputeType{1})
					return std::numeric_limits<To>::max();
				throwNumericOutOfRange();
			}

			return convertIntegralValue<To>(value);
		}

		template <FloatingNumber To, typename From>
		To numberToNumber(const ScaledNumber<From>& from, int toScale = 0)
		{
			assert(toScale == 0);

			using ComputeType = GreaterNumberType<double, To>;

			ComputeType value = convertFloatingValue<ComputeType>(from.value);

			if (from.scale != 0)
			{
				if (std::abs(from.scale) > std::numeric_limits<To>::max_exponent10)
					throwNumericOutOfRange();

				if (from.scale > 0)
					value *= powerOfTen<ComputeType>(from.scale);
				else if (from.scale < 0)
					value /= powerOfTen<ComputeType>(-from.scale);
			}

			return static_cast<To>(value);
		}

		template <FloatingNumber To, FloatingNumber From>
		To numberToNumber(const From& from, int toScale = 0)
		{
			assert(toScale == 0);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
			if constexpr (std::same_as<To, BoostDecFloat16> && std::same_as<From, BoostDecFloat34>)
				return boostDecFloat34ToBoostDecFloat16(from);
#endif

			return convertFloatingValue<To>(from);
		}

		template <IntegralNumber From>
		std::string numberToString(const ScaledNumber<From>& from)
		{
			char buffer[64];

			const bool isNegative = from.value < 0;
			const bool isMinLimit = from.value == std::numeric_limits<decltype(from.value)>::min();

			using UnsignedType = MakeUnsignedType<decltype(from.value)>;

			auto unsignedValue = isMinLimit ? static_cast<UnsignedType>(-(from.value + 1)) + 1
											: static_cast<UnsignedType>(isNegative ? -from.value : from.value);

			int digitCount = 0;

			do
			{
				buffer[digitCount++] = static_cast<char>((unsignedValue % 10) + '0');
				unsignedValue /= 10;
			} while (unsignedValue > 0);

			std::string result;

			if (isNegative)
				result += '-';

			if (from.scale >= 0)
			{
				for (int i = digitCount - 1; i >= 0; --i)
					result += buffer[i];

				result.append(static_cast<std::string::size_type>(from.scale), '0');
			}
			else
			{
				const int decimalPlaces = -from.scale;

				if (decimalPlaces >= static_cast<int>(digitCount))
				{
					result += "0.";
					const int leadingZeros = decimalPlaces - digitCount;
					result.append(static_cast<std::string::size_type>(leadingZeros), '0');

					for (int i = digitCount - 1; i >= 0; --i)
						result += buffer[i];
				}
				else
				{
					for (int i = digitCount - 1; i >= decimalPlaces; --i)
						result += buffer[i];

					result += '.';

					for (int i = decimalPlaces - 1; i >= 0; --i)
						result += buffer[i];
				}
			}

			return result;
		}

		template <FloatingNumber From>
		std::string numberToString(const From& from)
		{
			if constexpr (std::is_floating_point_v<From>)
			{
				if (std::isnan(from))
					return "NaN";
				if (std::isinf(from))
					return from > 0 ? "Infinity" : "-Infinity";
				return std::to_string(from);
			}
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
			else if constexpr (std::same_as<From, BoostDecFloat16> || std::same_as<From, BoostDecFloat34>)
			{
				if ((boost::multiprecision::isnan) (from))
					return "NaN";
				if ((boost::multiprecision::isinf) (from))
					return from > 0 ? "Infinity" : "-Infinity";
				return from.str();
			}
#endif
#if FB_CPP_USE_BOOST_DECIMAL != 0
			else if constexpr (std::same_as<From, BoostDecimal32> || std::same_as<From, BoostDecimal64> ||
				std::same_as<From, BoostDecimal128>)
			{
				if ((boost::decimal::isnan) (from))
					return "NaN";
				if ((boost::decimal::isinf) (from))
					return from > 0 ? "Infinity" : "-Infinity";
				return boost::decimal::to_string(from);
			}
#endif
			else
				return from.str();
		}

		std::string opaqueInt128ToString(StatusWrapper* statusWrapper, const OpaqueInt128& opaqueInt128, int scale)
		{
			const auto int128Util = client->getInt128Util(statusWrapper);
			char buffer[fb::IInt128::STRING_SIZE + 1];
			int128Util->toString(statusWrapper, &opaqueInt128, scale, static_cast<unsigned>(sizeof(buffer)), buffer);
			return buffer;
		}

		std::string opaqueDecFloat16ToString(StatusWrapper* statusWrapper, const OpaqueDecFloat16& opaqueDecFloat16)
		{
			const auto decFloat16Util = client->getDecFloat16Util(statusWrapper);
			char buffer[fb::IDecFloat16::STRING_SIZE + 1];
			decFloat16Util->toString(statusWrapper, &opaqueDecFloat16, static_cast<unsigned>(sizeof(buffer)), buffer);
			return buffer;
		}

		std::string opaqueDecFloat34ToString(StatusWrapper* statusWrapper, const OpaqueDecFloat34& opaqueDecFloat34)
		{
			const auto decFloat34Util = client->getDecFloat34Util(statusWrapper);
			char buffer[fb::IDecFloat34::STRING_SIZE + 1];
			decFloat34Util->toString(statusWrapper, &opaqueDecFloat34, static_cast<unsigned>(sizeof(buffer)), buffer);
			return buffer;
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		OpaqueInt128 boostInt128ToOpaqueInt128(StatusWrapper* statusWrapper, const BoostInt128& boostInt128)
		{
			validateFirebirdInt128(boostInt128);

			OpaqueInt128 opaqueInt128;
			const auto value = boostInt128.str();
			client->getInt128Util(statusWrapper)->fromString(statusWrapper, 0, value.c_str(), &opaqueInt128);

			return opaqueInt128;
		}

		OpaqueInt128 boostInt128ToOpaqueInt128(const BoostInt128& boostInt128)
		{
			StatusWrapper status{*client};
			return boostInt128ToOpaqueInt128(&status, boostInt128);
		}

		BoostInt128 opaqueInt128ToBoostInt128(StatusWrapper* statusWrapper, const OpaqueInt128& opaqueInt128)
		{
			return BoostInt128{opaqueInt128ToString(statusWrapper, opaqueInt128, 0)};
		}

		BoostInt128 opaqueInt128ToBoostInt128(const OpaqueInt128& opaqueInt128)
		{
			StatusWrapper status{*client};
			return opaqueInt128ToBoostInt128(&status, opaqueInt128);
		}

		OpaqueDecFloat16 boostDecFloat16ToOpaqueDecFloat16(
			StatusWrapper* statusWrapper, const BoostDecFloat16& boostDecFloat16)
		{
			const auto decFloat16Util = client->getDecFloat16Util(statusWrapper);
			OpaqueDecFloat16 opaqueDecFloat16;
			const auto value = numberToString(boostDecFloat16);
			decFloat16Util->fromString(statusWrapper, value.c_str(), &opaqueDecFloat16);
			return opaqueDecFloat16;
		}

		BoostDecFloat16 opaqueDecFloat16ToBoostDecFloat16(
			StatusWrapper* statusWrapper, const OpaqueDecFloat16& opaqueDecFloat16)
		{
			const auto value = opaqueDecFloat16ToString(statusWrapper, opaqueDecFloat16);

			if (isSignalingNaN(value))
				throw FbCppException("BoostDecFloat16 cannot represent a signaling NaN");
			else if (value == "Infinity")
				return std::numeric_limits<BoostDecFloat16>::infinity();
			else if (value == "-Infinity")
				return -std::numeric_limits<BoostDecFloat16>::infinity();

			try
			{
				return BoostDecFloat16{value};
			}
			catch (const std::exception&)
			{
				throwConversionErrorFromString(value);
			}
		}

		OpaqueDecFloat34 boostDecFloat34ToOpaqueDecFloat34(
			StatusWrapper* statusWrapper, const BoostDecFloat34& boostDecFloat34)
		{
			const auto decFloat34Util = client->getDecFloat34Util(statusWrapper);
			OpaqueDecFloat34 opaqueDecFloat34;
			const auto value = numberToString(boostDecFloat34);
			decFloat34Util->fromString(statusWrapper, value.c_str(), &opaqueDecFloat34);
			return opaqueDecFloat34;
		}

		BoostDecFloat34 opaqueDecFloat34ToBoostDecFloat34(
			StatusWrapper* statusWrapper, const OpaqueDecFloat34& opaqueDecFloat34)
		{
			const auto value = opaqueDecFloat34ToString(statusWrapper, opaqueDecFloat34);

			if (isSignalingNaN(value))
				throw FbCppException("BoostDecFloat34 cannot represent a signaling NaN; use OpaqueDecFloat34");
			else if (value == "Infinity")
				return std::numeric_limits<BoostDecFloat34>::infinity();
			else if (value == "-Infinity")
				return -std::numeric_limits<BoostDecFloat34>::infinity();

			try
			{
				return BoostDecFloat34{value};
			}
			catch (const std::exception&)
			{
				throwConversionErrorFromString(value);
			}
		}

		BoostDecFloat16 boostDecFloat34ToBoostDecFloat16(const BoostDecFloat34& boostDecFloat34)
		{
			StatusWrapper status{*client};
			OpaqueDecFloat16 opaqueDecFloat16;
			const auto value = numberToString(boostDecFloat34);
			client->getDecFloat16Util(&status)->fromString(&status, value.c_str(), &opaqueDecFloat16);
			return opaqueDecFloat16ToBoostDecFloat16(&status, opaqueDecFloat16);
		}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		BoostDecimal32 opaqueDecFloat16ToBoostDecimal32(
			StatusWrapper* statusWrapper, const OpaqueDecFloat16& opaqueDecFloat16)
		{
			return stringToBoostDecimal<BoostDecimal32>(opaqueDecFloat16ToString(statusWrapper, opaqueDecFloat16));
		}

		BoostDecimal64 opaqueDecFloat16ToBoostDecimal64(
			StatusWrapper* statusWrapper, const OpaqueDecFloat16& opaqueDecFloat16)
		{
			return stringToBoostDecimal<BoostDecimal64>(opaqueDecFloat16ToString(statusWrapper, opaqueDecFloat16));
		}

		BoostDecimal128 opaqueDecFloat16ToBoostDecimal128(
			StatusWrapper* statusWrapper, const OpaqueDecFloat16& opaqueDecFloat16)
		{
			return stringToBoostDecimal<BoostDecimal128>(opaqueDecFloat16ToString(statusWrapper, opaqueDecFloat16));
		}

		BoostDecimal32 opaqueDecFloat34ToBoostDecimal32(
			StatusWrapper* statusWrapper, const OpaqueDecFloat34& opaqueDecFloat34)
		{
			return stringToBoostDecimal<BoostDecimal32>(opaqueDecFloat34ToString(statusWrapper, opaqueDecFloat34));
		}

		BoostDecimal64 opaqueDecFloat34ToBoostDecimal64(
			StatusWrapper* statusWrapper, const OpaqueDecFloat34& opaqueDecFloat34)
		{
			return stringToBoostDecimal<BoostDecimal64>(opaqueDecFloat34ToString(statusWrapper, opaqueDecFloat34));
		}

		BoostDecimal128 opaqueDecFloat34ToBoostDecimal128(
			StatusWrapper* statusWrapper, const OpaqueDecFloat34& opaqueDecFloat34)
		{
			return stringToBoostDecimal<BoostDecimal128>(opaqueDecFloat34ToString(statusWrapper, opaqueDecFloat34));
		}

		OpaqueDecFloat16 boostDecimal32ToOpaqueDecFloat16(StatusWrapper* statusWrapper, const BoostDecimal32& value)
		{
			return boostDecimalToOpaqueDecFloat16(statusWrapper, value);
		}

		OpaqueDecFloat16 boostDecimal64ToOpaqueDecFloat16(StatusWrapper* statusWrapper, const BoostDecimal64& value)
		{
			return boostDecimalToOpaqueDecFloat16(statusWrapper, value);
		}

		OpaqueDecFloat16 boostDecimal128ToOpaqueDecFloat16(StatusWrapper* statusWrapper, const BoostDecimal128& value)
		{
			return boostDecimalToOpaqueDecFloat16(statusWrapper, value);
		}

		OpaqueDecFloat34 boostDecimal32ToOpaqueDecFloat34(StatusWrapper* statusWrapper, const BoostDecimal32& value)
		{
			return boostDecimalToOpaqueDecFloat34(statusWrapper, value);
		}

		OpaqueDecFloat34 boostDecimal64ToOpaqueDecFloat34(StatusWrapper* statusWrapper, const BoostDecimal64& value)
		{
			return boostDecimalToOpaqueDecFloat34(statusWrapper, value);
		}

		OpaqueDecFloat34 boostDecimal128ToOpaqueDecFloat34(StatusWrapper* statusWrapper, const BoostDecimal128& value)
		{
			return boostDecimalToOpaqueDecFloat34(statusWrapper, value);
		}
#endif

		// FIXME: move
		std::byte stringToBoolean(std::string_view value)
		{
			auto trimmed = value;

			while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
				trimmed.remove_prefix(1);

			while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
				trimmed.remove_suffix(1);

			std::string normalized{trimmed};
			std::transform(normalized.begin(), normalized.end(), normalized.begin(),
				[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

			if (normalized == "true")
				return std::byte{1};
			else if (normalized == "false")
				return std::byte{0};

			throwConversionErrorFromString(std::string{value});
		}

	private:
		template <typename T>
		static bool isNonFinite(const T& value)
		{
			using ValueType = std::remove_cvref_t<T>;

			if constexpr (std::is_floating_point_v<ValueType>)
				return std::isnan(value) || std::isinf(value);
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
			else if constexpr (std::same_as<ValueType, BoostDecFloat16> || std::same_as<ValueType, BoostDecFloat34>)
				return (boost::multiprecision::isnan) (value) || (boost::multiprecision::isinf) (value);
#endif
#if FB_CPP_USE_BOOST_DECIMAL != 0
			else if constexpr (std::same_as<ValueType, BoostDecimal32> || std::same_as<ValueType, BoostDecimal64> ||
				std::same_as<ValueType, BoostDecimal128>)
				return (boost::decimal::isnan) (value) || (boost::decimal::isinf) (value);
#endif
			else
				return false;
		}

		template <typename T>
		static bool isSignalingNaNValue(const T& value)
		{
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0 || FB_CPP_USE_BOOST_DECIMAL != 0
			using ValueType = std::remove_cvref_t<T>;
#endif

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
			if constexpr (std::same_as<ValueType, BoostDecFloat16> || std::same_as<ValueType, BoostDecFloat34>)
				return (boost::multiprecision::isnan) (value) && isSignalingNaN(value.str());
#endif
#if FB_CPP_USE_BOOST_DECIMAL != 0
			if constexpr (std::same_as<ValueType, BoostDecimal32> || std::same_as<ValueType, BoostDecimal64> ||
				std::same_as<ValueType, BoostDecimal128>)
				return (boost::decimal::issignaling) (value);
#endif
			return false;
		}

		template <typename To, typename From>
		To convertFloatingValue(const From& from)
		{
			using ToType = std::remove_cvref_t<To>;
			using FromType = std::remove_cvref_t<From>;

			if constexpr (std::same_as<ToType, FromType>)
				return from;
#if FB_CPP_USE_BOOST_DECIMAL != 0
			else if constexpr (std::same_as<ToType, BoostDecimal32> || std::same_as<ToType, BoostDecimal64> ||
				std::same_as<ToType, BoostDecimal128>)
			{
				if (isSignalingNaNValue(from))
					return std::numeric_limits<ToType>::signaling_NaN();
				if constexpr (std::is_floating_point_v<FromType>)
					return ToType{std::format("{:.16e}", from)};
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				else if constexpr (std::same_as<FromType, BoostDecFloat16> || std::same_as<FromType, BoostDecFloat34>)
				{
					if ((boost::multiprecision::isnan) (from))
						return std::numeric_limits<ToType>::quiet_NaN();
					if ((boost::multiprecision::isinf) (from))
					{
						return from > 0 ? std::numeric_limits<ToType>::infinity()
										: -std::numeric_limits<ToType>::infinity();
					}
					return ToType{from.str()};
				}
				else if constexpr (std::same_as<FromType, BoostInt128>)
					return ToType{from.str()};
#endif
				else
					return static_cast<ToType>(from);
			}
#endif
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0 && FB_CPP_USE_BOOST_DECIMAL != 0
			else if constexpr ((std::same_as<ToType, BoostDecFloat16> || std::same_as<ToType, BoostDecFloat34>) &&
				(std::same_as<FromType, BoostDecimal32> || std::same_as<FromType, BoostDecimal64> ||
					std::same_as<FromType, BoostDecimal128>) )
			{
				if (isSignalingNaNValue(from))
					throw FbCppException("Boost.Multiprecision cannot represent a signaling NaN");
				if ((boost::decimal::isnan) (from))
					return ToType{"NaN"};
				if ((boost::decimal::isinf) (from))
				{
					return from > 0 ? std::numeric_limits<ToType>::infinity()
									: -std::numeric_limits<ToType>::infinity();
				}
				return ToType{boost::decimal::to_string(from)};
			}
#endif
			else
				return static_cast<ToType>(from);
		}

		template <typename To, typename From>
		To convertIntegralValue(const From& from)
		{
			using ToType = std::remove_cvref_t<To>;

#if FB_CPP_USE_BOOST_DECIMAL != 0
			using FromType = std::remove_cvref_t<From>;

			if constexpr (std::same_as<FromType, BoostDecimal32> || std::same_as<FromType, BoostDecimal64> ||
				std::same_as<FromType, BoostDecimal128>)
			{
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				if constexpr (std::same_as<ToType, BoostInt128>)
				{
					char buffer[64];
					const auto result = boost::decimal::to_chars(
						buffer, buffer + sizeof(buffer), from, boost::decimal::chars_format::fixed);

					if (result.ec != std::errc{})
						throwNumericOutOfRange();

					std::string value{buffer, result.ptr};
					if (const auto decimalPoint = value.find('.'); decimalPoint != std::string::npos)
						value.erase(decimalPoint);

					return ToType{value};
				}
#endif

				if constexpr (std::same_as<ToType, bool>)
					return static_cast<ToType>(static_cast<bool>(from));
				else if constexpr (std::is_signed_v<ToType>)
					return static_cast<ToType>(static_cast<long long>(from));
				else
					return static_cast<ToType>(static_cast<unsigned long long>(from));
			}
			else
#endif
				return static_cast<ToType>(from);
		}

		static bool isSignalingNaN(std::string_view value)
		{
			std::string normalized{value};
			if (!normalized.empty() && (normalized.front() == '+' || normalized.front() == '-'))
				normalized.erase(0, 1);

			std::transform(normalized.begin(), normalized.end(), normalized.begin(),
				[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

			return normalized.starts_with("snan");
		}

#if FB_CPP_USE_BOOST_DECIMAL != 0
		template <typename T>
		T stringToBoostDecimal(const std::string& value)
		{
			if (isSignalingNaN(value))
				return std::numeric_limits<T>::signaling_NaN();
			else if (value == "Infinity")
				return std::numeric_limits<T>::infinity();
			else if (value == "-Infinity")
				return -std::numeric_limits<T>::infinity();

			try
			{
				return T{value};
			}
			catch (const std::exception&)
			{
				throwConversionErrorFromString(value);
			}
		}

		template <typename T>
		OpaqueDecFloat16 boostDecimalToOpaqueDecFloat16(StatusWrapper* statusWrapper, const T& value)
		{
			if (isSignalingNaNValue(value))
				throw FbCppException("Boost.Decimal cannot represent a signaling NaN; use OpaqueDecFloat16");

			OpaqueDecFloat16 result;
			const auto stringValue = numberToString(value);
			client->getDecFloat16Util(statusWrapper)->fromString(statusWrapper, stringValue.c_str(), &result);
			return result;
		}

		template <typename T>
		OpaqueDecFloat34 boostDecimalToOpaqueDecFloat34(StatusWrapper* statusWrapper, const T& value)
		{
			if (isSignalingNaNValue(value))
				throw FbCppException("Boost.Decimal cannot represent a signaling NaN; use OpaqueDecFloat34");

			OpaqueDecFloat34 result;
			const auto stringValue = numberToString(value);
			client->getDecFloat34Util(statusWrapper)->fromString(statusWrapper, stringValue.c_str(), &result);
			return result;
		}
#endif

		double powerOfTenDouble(int scale) noexcept
		{
			static constexpr double UPPER_PART[] = {
				1.e000,
				1.e032,
				1.e064,
				1.e096,
				1.e128,
				1.e160,
				1.e192,
				1.e224,
				1.e256,
				1.e288,
			};

			static constexpr double LOWER_PART[] = {
				1.e00,
				1.e01,
				1.e02,
				1.e03,
				1.e04,
				1.e05,
				1.e06,
				1.e07,
				1.e08,
				1.e09,
				1.e10,
				1.e11,
				1.e12,
				1.e13,
				1.e14,
				1.e15,
				1.e16,
				1.e17,
				1.e18,
				1.e19,
				1.e20,
				1.e21,
				1.e22,
				1.e23,
				1.e24,
				1.e25,
				1.e26,
				1.e27,
				1.e28,
				1.e29,
				1.e30,
				1.e31,
			};

			assert((scale >= 0) && (scale < 320));

			const auto upper = UPPER_PART[scale >> 5];
			const auto lower = LOWER_PART[scale & 0x1F];

			return upper * lower;
		}

		template <typename T>
		T powerOfTen(int scale)
		{
			assert((scale >= 0) && (scale < 320));

			if constexpr (std::same_as<T, float> || std::same_as<T, double>)
				return static_cast<T>(powerOfTenDouble(scale));
			else
			{
				T result{1};

				for (int i = 0; i < scale; ++i)
					result *= 10;

				return result;
			}
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		void validateFirebirdInt128(const BoostInt128& value)
		{
			constexpr BoostInt128 FB_MAX_INT128 = (BoostInt128{1} << 127) - 1;
			constexpr BoostInt128 FB_MIN_INT128 = -(BoostInt128{1} << 127);

			if (value < FB_MIN_INT128 || value > FB_MAX_INT128)
				throwNumericOutOfRange();
		}

#endif

		template <typename T>
		void adjustScale(T& val, int scale, const T minLimit, const T maxLimit)
		{
			if (scale > 0)
			{
				int fraction = 0;

				do
				{
					if (scale == 1)
						fraction = int(val % 10);
					val /= 10;
				} while (--scale);

				if (fraction > 4)
					++val;
				else if (fraction < -4)
				{
					static_assert((-85 / 10 == -8) && (-85 % 10 == -5),
						"If we port to a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)), we'll have to change "
						"this depending on the platform");
					--val;
				}
			}
			else if (scale < 0)
			{
				do
				{
					if ((val > maxLimit / 10) || (val < minLimit / 10))
						throwNumericOutOfRange();

					val *= 10;

					if (val > maxLimit || val < minLimit)
						throwNumericOutOfRange();
				} while (++scale);
			}
		}

		template <typename T>
		T conversionEpsilon()
		{
			if constexpr (std::is_same_v<T, float>)
				return static_cast<T>(1e-5f);
			else if constexpr (std::is_same_v<T, double>)
				return static_cast<T>(1e-14);
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
			else if constexpr (std::same_as<T, BoostDecFloat16> || std::same_as<T, BoostDecFloat34>)
			{
				const auto epsilon = std::numeric_limits<T>::epsilon();
				return static_cast<T>(epsilon * static_cast<T>(10));
			}
#endif
#if FB_CPP_USE_BOOST_DECIMAL != 0
			else if constexpr (std::same_as<T, BoostDecimal32> || std::same_as<T, BoostDecimal64> ||
				std::same_as<T, BoostDecimal128>)
			{
				const auto epsilon = std::numeric_limits<T>::epsilon();
				return epsilon * static_cast<T>(10);
			}
#endif
			else
				return std::numeric_limits<T>::epsilon();
		}

	private:
		Client* client;
	};
}  // namespace fbcpp::impl


#endif  // FBCPP_NUMERIC_CONVERTER_H
