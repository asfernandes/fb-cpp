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

#include "TestUtil.h"
#include "CalendarConverter.h"
#include "Exception.h"
#include "types.h"
#include <array>
#include <chrono>
#include <optional>
#include <ostream>
#include <string_view>
#include <boost/test/data/test_case.hpp>


namespace data = boost::unit_test::data;
using namespace std::chrono;
using namespace std::chrono_literals;


static const auto spTz = locate_zone("America/Sao_Paulo");
static const auto utcTz = locate_zone("UTC");

namespace
{
	template <typename T>
	concept TestCase = requires(T t) { t.outputText; };

	template <TestCase T>
	std::ostream& operator<<(std::ostream& os, const T& testCase)
	{
		return os << testCase.inputText.value_or(testCase.outputText);
	}

	struct DateCase final
	{
		Date date;
		std::string_view outputText;
		std::optional<std::string_view> inputText = std::nullopt;
	};

	struct TimeCase final
	{
		Time time;
		std::string_view outputText;
		std::optional<std::string_view> inputText = std::nullopt;
	};

	struct TimestampCase final
	{
		Timestamp timestamp;
		std::string_view outputText;
		std::optional<std::string_view> inputText = std::nullopt;
	};

	struct TimeTzCase final
	{
		Time time;
		std::string_view zone;
		std::string_view outputText;
		std::optional<std::string_view> inputText = std::nullopt;
	};

	struct TimeTzOffsetCase final
	{
		std::string_view outputText;
		std::optional<std::string_view> inputText = std::nullopt;
	};

	struct TimestampTzCase final
	{
		Timestamp timestamp;
		std::string_view zone;
		std::string_view outputText;
		std::optional<std::string_view> inputText = std::nullopt;
	};

	struct TimestampTzOffsetCase final
	{
		std::string_view outputText;
		std::optional<std::string_view> inputText = std::nullopt;
	};
}  // namespace


BOOST_AUTO_TEST_SUITE(CalendarConverterSuite)


static const std::initializer_list<DateCase> DATE_CASES{
	{2024y / month{2} / 29d, "2024-02-29"},
	{1y / month{1} / 1d, "0001-01-01"},
	{9999y / month{12} / 31d, "9999-12-31", " 9999 - 12 - 31 "},
};

BOOST_DATA_TEST_CASE(dateConversion, data::make(DATE_CASES), dateCase)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const auto& date = dateCase.date;
	const std::string outputText{dateCase.outputText};
	const std::string inputText{dateCase.inputText.value_or(outputText)};

	const auto opaque = converter.dateToOpaqueDate(date);
	BOOST_CHECK_EQUAL(converter.opaqueDateToDate(opaque), date);
	BOOST_CHECK_EQUAL(converter.opaqueDateToString(opaque), outputText);

	const auto parsed = converter.stringToDate(inputText);
	BOOST_CHECK_EQUAL(parsed, date);

	const auto parsedOpaque = converter.stringToOpaqueDate(inputText);
	BOOST_CHECK_EQUAL(converter.opaqueDateToDate(parsedOpaque), date);
}


static const std::initializer_list<TimeCase> TIME_CASES{
	{Time{13h + 14min + 15s + 123400us}, "13:14:15.1234"},
	{Time{0us}, "00:00:00.0000"},
	{Time{23h + 59min + 59s + 999900us}, "23:59:59.9999", "  23 : 59 : 59 . 9999  "},
};

BOOST_DATA_TEST_CASE(timeConversion, data::make(TIME_CASES), timeCase)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const Time time{timeCase.time};
	const std::string outputText{timeCase.outputText};
	const std::string inputText{timeCase.inputText.value_or(outputText)};

	const auto opaque = converter.timeToOpaqueTime(time);
	BOOST_CHECK_EQUAL(converter.opaqueTimeToTime(opaque).to_duration(), time.to_duration());
	BOOST_CHECK_EQUAL(converter.opaqueTimeToString(opaque), outputText);

	const auto parsed = converter.stringToTime(inputText);
	BOOST_CHECK_EQUAL(parsed.to_duration(), time.to_duration());

	const auto parsedOpaque = converter.stringToOpaqueTime(inputText);
	BOOST_CHECK_EQUAL(converter.opaqueTimeToTime(parsedOpaque).to_duration(), time.to_duration());
}


static const std::initializer_list<TimestampCase> TIMESTAMP_CASES{
	{Timestamp{local_days{2024y / month{2} / 29d} + 13h + 14min + 15s + 123400us}, "2024-02-29 13:14:15.1234"},
	{Timestamp{local_days{1y / month{1} / 1d} + microseconds::zero()}, "0001-01-01 00:00:00.0000"},
	{Timestamp{local_days{9999y / month{12} / 31d} + 23h + 59min + 59s + 999900us}, "9999-12-31 23:59:59.9999",
		"  9999 - 12 - 31    23 : 59 : 59 . 9999  "},
};

BOOST_DATA_TEST_CASE(timestampConversion, data::make(TIMESTAMP_CASES), timestampCase)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const auto timestamp = timestampCase.timestamp;
	const std::string outputText{timestampCase.outputText};
	const std::string inputText{timestampCase.inputText.value_or(outputText)};

	const auto opaque = converter.timestampToOpaqueTimestamp(timestamp);
	BOOST_CHECK(converter.opaqueTimestampToTimestamp(opaque) == timestamp);
	BOOST_CHECK_EQUAL(converter.opaqueTimestampToString(opaque), outputText);

	const auto parsed = converter.stringToTimestamp(inputText);
	BOOST_CHECK(parsed == timestamp);

	const auto parsedOpaque = converter.stringToOpaqueTimestamp(inputText);
	BOOST_CHECK(converter.opaqueTimestampToTimestamp(parsedOpaque) == timestamp);
}


static const std::initializer_list<TimeTzCase> TIME_TZ_CASES{
	{Time{13h + 14min + 15s + 123400us}, "America/Sao_Paulo", "13:14:15.1234 America/Sao_Paulo"},
	{Time{0us}, "America/Sao_Paulo", "00:00:00.0000 America/Sao_Paulo"},
	{Time{23h + 59min + 59s + 999900us}, "America/Sao_Paulo", "23:59:59.9999 America/Sao_Paulo",
		"  23 : 59 : 59 . 9999    America/Sao_Paulo  "},
};

BOOST_DATA_TEST_CASE(timeTzConversion, data::make(TIME_TZ_CASES), timeTzCase)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const Date baseTimeTzDate{year{2020}, month{1}, 1d};
	const Timestamp localTimestamp{local_days{baseTimeTzDate} + timeTzCase.time.to_duration()};
	const auto localTimestampUtc = utcTz->to_local(spTz->to_sys(localTimestamp));
	const TimeTz timeTz{Time{localTimestampUtc - floor<days>(localTimestampUtc)}, std::string{timeTzCase.zone}};
	const std::string outputText{timeTzCase.outputText};
	const std::string inputText{timeTzCase.inputText.value_or(outputText)};

	const auto opaque = converter.timeTzToOpaqueTimeTz(timeTz);
	BOOST_CHECK(converter.opaqueTimeTzToTimeTz(opaque) == timeTz);
	BOOST_CHECK_EQUAL(converter.opaqueTimeTzToString(opaque), outputText);

	const auto parsed = converter.stringToTimeTz(inputText);
	BOOST_CHECK(parsed == timeTz);

	const auto parsedOpaque = converter.stringToOpaqueTimeTz(inputText);
	BOOST_CHECK(converter.opaqueTimeTzToTimeTz(parsedOpaque) == timeTz);
}


static const std::initializer_list<TimeTzOffsetCase> TIME_TZ_OFFSET_CASES{
	{"11:14:15.1234 -03:00"},
	{"12:14:15.1234 +03:00"},
	{"  11 : 14 : 15 . 1234  - 03 : 00 ", "11:14:15.1234 -03:00"},
};

BOOST_DATA_TEST_CASE(timeTzOffsetConversion, data::make(TIME_TZ_OFFSET_CASES), timeTzOffsetCase)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const std::string outputText{timeTzOffsetCase.outputText};
	const std::string inputText{timeTzOffsetCase.inputText.value_or(outputText)};

	const auto parsedOpaque = converter.stringToOpaqueTimeTz(inputText);
	const auto text = converter.opaqueTimeTzToString(parsedOpaque);
	BOOST_CHECK_EQUAL(text, inputText);
}


static const std::initializer_list<TimestampTzCase> TIMESTAMP_TZ_CASES{
	{Timestamp{local_days{2024y / month{2} / 29d} + 13h + 14min + 15s + 123400us}, "America/Sao_Paulo",
		"2024-02-29 13:14:15.1234 America/Sao_Paulo"},
	{Timestamp{local_days{1y / month{1} / 1d} + 13h + 14min + 15s + 123400us}, "UTC", "0001-01-01 13:14:15.1234 UTC"},
	{Timestamp{local_days{9999y / month{12} / 31d} + 23h + 59min + 59s + 999900us}, "America/Sao_Paulo",
		"9999-12-31 23:59:59.9999 America/Sao_Paulo", "  9999 - 12 - 31   23 : 59 : 59 . 9999    America/Sao_Paulo  "},
};

BOOST_DATA_TEST_CASE(timestampTzConversion, data::make(TIMESTAMP_TZ_CASES), timestampTzCase)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	auto utcTimestamp = timestampTzCase.timestamp;
	if (timestampTzCase.zone != "UTC")
		utcTimestamp = utcTz->to_local(spTz->to_sys(timestampTzCase.timestamp));
	const TimestampTz timestampTz{utcTimestamp, std::string{timestampTzCase.zone}};
	const std::string outputText{timestampTzCase.outputText};
	const std::string inputText{timestampTzCase.inputText.value_or(outputText)};

	const auto opaque = converter.timestampTzToOpaqueTimestampTz(timestampTz);
	BOOST_CHECK(converter.opaqueTimestampTzToTimestampTz(opaque) == timestampTz);
	BOOST_CHECK_EQUAL(converter.opaqueTimestampTzToString(opaque), outputText);

	const auto parsed = converter.stringToTimestampTz(inputText);
	BOOST_CHECK(parsed == timestampTz);

	const auto parsedOpaque = converter.stringToOpaqueTimestampTz(inputText);
	BOOST_CHECK(converter.opaqueTimestampTzToTimestampTz(parsedOpaque) == timestampTz);
}


static const std::initializer_list<TimestampTzOffsetCase> TIMESTAMP_TZ_OFFSET_CASES{
	{"2024-02-29 11:14:15.1234 -03:00"},
	{"2024-02-29 12:14:15.1234 +03:00"},
	{"  2024 - 02 - 01  11 : 14 : 15 . 1234  -03 : 00 ", "2024-02-01 11:14:15.1234 -03:00"},
};

BOOST_DATA_TEST_CASE(timestampTzOffsetConversion, data::make(TIMESTAMP_TZ_OFFSET_CASES), timestampTzOffsetCase)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const std::string outputText{timestampTzOffsetCase.outputText};
	const std::string inputText{timestampTzOffsetCase.inputText.value_or(outputText)};

	const auto parsedOpaque = converter.stringToOpaqueTimestampTz(inputText);
	const auto text = converter.opaqueTimestampTzToString(parsedOpaque);
	BOOST_CHECK_EQUAL(text, inputText);
}


BOOST_AUTO_TEST_SUITE_END()
