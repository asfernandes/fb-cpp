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

#ifndef FBCPP_TYPES_H
#define FBCPP_TYPES_H

#include "fb-api.h"
#include "config.h"
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	template <typename T>
	struct ScaledNumber final
	{
		bool operator==(const ScaledNumber&) const noexcept = default;

		T value{};
		int scale = 0;
	};

	using ScaledInt16 = ScaledNumber<std::int16_t>;
	using ScaledInt32 = ScaledNumber<std::int32_t>;
	using ScaledInt64 = ScaledNumber<std::int64_t>;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	using BoostInt128 = boost::multiprecision::int128_t;
	using ScaledBoostInt128 = ScaledNumber<BoostInt128>;
#endif

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	using BoostDecFloat16 = boost::multiprecision::number<boost::multiprecision::cpp_dec_float<16>>;
	using BoostDecFloat34 = boost::multiprecision::number<boost::multiprecision::cpp_dec_float<34>>;
#endif

	using Date = std::chrono::year_month_day;
	using Time = std::chrono::hh_mm_ss<std::chrono::microseconds>;
	using Timestamp = std::chrono::local_time<std::chrono::microseconds>;  // FIXME: MSVC support range

	struct TimeTz final
	{
		bool operator==(const TimeTz& other) const noexcept
		{
			return utcTime.to_duration() == other.utcTime.to_duration() && zone == other.zone;
		}

		Time utcTime;
		std::string zone;
	};

	struct TimestampTz final
	{
		bool operator==(const TimestampTz&) const noexcept = default;

		Timestamp utcTimestamp;
		std::string zone;
	};

	using OpaqueInt128 = FB_I128;
	using OpaqueDecFloat16 = FB_DEC16;
	using OpaqueDecFloat34 = FB_DEC34;
	using ScaledOpaqueInt128 = ScaledNumber<OpaqueInt128>;

	struct alignas(alignof(ISC_DATE)) OpaqueDate final
	{
		bool operator==(const OpaqueDate&) const noexcept = default;

		ISC_DATE value;
	};

	struct alignas(alignof(ISC_TIME)) OpaqueTime final
	{
		bool operator==(const OpaqueTime&) const noexcept = default;

		ISC_TIME value;
	};

	struct alignas(alignof(ISC_TIMESTAMP)) OpaqueTimestamp final
	{
		bool operator==(const OpaqueTimestamp& other) const noexcept
		{
			return value.timestamp_date == other.value.timestamp_date &&
				value.timestamp_time == other.value.timestamp_time;
		}

		ISC_TIMESTAMP value;
	};

	struct alignas(alignof(ISC_TIME_TZ)) OpaqueTimeTz final
	{
		bool operator==(const OpaqueTimeTz&) const noexcept = default;

		ISC_TIME_TZ value;
	};

	struct alignas(alignof(ISC_TIMESTAMP_TZ)) OpaqueTimestampTz final
	{
		bool operator==(const OpaqueTimestampTz&) const noexcept = default;

		ISC_TIMESTAMP_TZ value;
	};

	// FIXME: test
	template <typename T>
	std::ostream& operator<<(std::ostream& os, const fbcpp::ScaledNumber<T>& scaledNumber)
	{
		os << scaledNumber.value << "e" << scaledNumber.scale;
		return os;
	}
}  // namespace fbcpp


// FIXME: test
template <typename T>
struct std::formatter<fbcpp::ScaledNumber<T>> : std::formatter<T>
{
	constexpr auto parse(format_parse_context& ctx)
	{
		return std::formatter<T>::parse(ctx);
	}

	template <typename FormatContext>
	auto format(const fbcpp::ScaledNumber<T>& scaledNumber, FormatContext& ctx) const
	{
		return std::format_to(
			ctx.out(), "{}e{}", std::formatter<T>::format(scaledNumber.value, ctx), scaledNumber.scale);
	}
};


#endif  // FBCPP_TYPES_H
