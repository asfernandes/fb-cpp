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
#include "Statement.h"
#include "Transaction.h"
#include "Exception.h"
#include <chrono>
#include <exception>
#include <string>
#include <string_view>


// FIXME: Refactor tests
// FIXME: +inf, -inf, +nan, -nan, etc tests
BOOST_AUTO_TEST_SUITE(StatementSuite)

BOOST_AUTO_TEST_CASE(constructor)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-constructor.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement statement{attachment, transaction, "create table t (n integer)"};
}

BOOST_AUTO_TEST_CASE(constructorWithSetTransaction)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-constructorSetTransaction.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	BOOST_CHECK_THROW(Statement(attachment, transaction, "set transaction isolation level snapshot"), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(constructorWithCommit)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-constructorCommit.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	BOOST_CHECK_THROW(Statement(attachment, transaction, "commit"), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(constructorWithRollback)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-constructorRollback.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	BOOST_CHECK_THROW(Statement(attachment, transaction, "rollback"), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(free)
{
	Attachment attachment{CLIENT, getTempFile("Statement-free.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, "create table t (n integer)"};
	BOOST_CHECK_EQUAL(statement.isValid(), true);

	statement.free();
	BOOST_CHECK_EQUAL(statement.isValid(), false);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(isNotValidAfterMove)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-isNotValidAfterMove.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, "create table t (n integer)"};
	BOOST_CHECK_EQUAL(statement.isValid(), true);

	Statement statement2 = std::move(statement);
	BOOST_CHECK_EQUAL(statement2.isValid(), true);
	BOOST_CHECK_EQUAL(statement.isValid(), false);

	statement2.free();
	BOOST_CHECK_EQUAL(statement2.isValid(), false);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(getLegacyPlan)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-getLegacyPlan.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement statement{attachment, transaction, "select 1 from rdb$database"};

	const auto plan = statement.getLegacyPlan();
	BOOST_CHECK(plan == "\nPLAN (RDB$DATABASE NATURAL)" || plan == "\nPLAN (\"SYSTEM\".\"RDB$DATABASE\" NATURAL)");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(getPlan)
{
	Attachment attachment{CLIENT, getTempFile("Statement-getPlan.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement statement{attachment, transaction, "select 1 from rdb$database"};

	const auto plan = statement.getPlan();
	BOOST_CHECK(plan == "\nSelect Expression\n    -> Table \"RDB$DATABASE\" Full Scan" ||
		plan == "\nSelect Expression\n    -> Table \"SYSTEM\".\"RDB$DATABASE\" Full Scan");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(getType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-getType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddlStatement{attachment, transaction, "create table t (n integer)"};
	BOOST_CHECK(ddlStatement.getType() == StatementType::DDL);

	Statement selectStatement{attachment, transaction, "select 1 from rdb$database"};
	BOOST_CHECK(selectStatement.getType() == StatementType::SELECT);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(execute)
{
	Attachment attachment{CLIENT, getTempFile("Statement-execute.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement statement{attachment, transaction, "create table t (n integer)"};

	BOOST_CHECK(statement.execute(transaction));
	transaction.commit();
}

BOOST_AUTO_TEST_CASE(nullType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-nullType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, "select null, 1 from rdb$database where cast(? as boolean) is null"};

	statement.setNull(0);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.isNull(0));
	BOOST_CHECK(!statement.isNull(1));

	statement.setBool(0, true);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(statement.isNull(0));
	BOOST_CHECK(statement.isNull(1));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(boolType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-boolType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{
		attachment, transaction, "select true, false from rdb$database where cast(? as boolean) is true"};

	statement.setBool(0, true);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getBool(0).value());
	BOOST_CHECK(!statement.getBool(1).value());

	statement.setBool(0, false);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getBool(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(int16Type)
{
	Attachment attachment{CLIENT, getTempFile("Statement-int16Type.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, "select 1, 2 from rdb$database where 100 = cast(? as smallint)"};

	statement.setInt16(0, 100);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK_EQUAL(statement.getInt16(0).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt16(1).value(), 2);

	statement.setInt16(0, 0);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getInt16(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(scaledInt16Type)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-scaledInt16Type.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, "select 1, 2, 3.45 from rdb$database where 100 = cast(? as smallint)"};

	statement.setScaledInt16(0, ScaledInt16{100, 0});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getScaledInt16(0).value() == ScaledInt16{1, 0}));
	BOOST_CHECK((statement.getScaledInt16(1).value() == ScaledInt16{2, 0}));
	BOOST_CHECK((statement.getScaledInt16(2).value() == ScaledInt16{345, -2}));

	statement.setScaledInt16(0, ScaledInt16{0, 0});
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getScaledInt16(0).has_value());

	statement.setScaledInt16(0, ScaledInt16{1000, -1});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getScaledInt16(0).value() == ScaledInt16{1, 0}));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(int32Type)
{
	Attachment attachment{CLIENT, getTempFile("Statement-int32Type.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select 100000, 200000, 300000.45, 12345.67890123 from rdb$database where 10000000 = cast(? as integer)"};

	statement.setInt32(0, 10000000);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getInt32(0).value() == 100000));
	BOOST_CHECK((statement.getInt32(1).value() == 200000));
	BOOST_CHECK((statement.getInt32(2).value() == 300000));
	BOOST_CHECK((statement.getInt32(3).value() == 12346));

	statement.setInt32(0, 0);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getInt32(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(scaledInt32Type)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-scaledInt32Type.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select 100000, 200000, 300000.45 from rdb$database where 10000000 = cast(? as integer)"};

	statement.setScaledInt32(0, ScaledInt32{10000000, 0});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getScaledInt32(0).value() == ScaledInt32{100000, 0}));
	BOOST_CHECK((statement.getScaledInt32(1).value() == ScaledInt32{200000, 0}));
	BOOST_CHECK((statement.getScaledInt32(2).value() == ScaledInt32{30000045, -2}));

	statement.setScaledInt32(0, ScaledInt32{0, 0});
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getScaledInt32(0).has_value());

	statement.setScaledInt32(0, ScaledInt32{100000000, -1});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getScaledInt32(0).value() == ScaledInt32{100000, 0}));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(scaledInt64Type)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-scaledInt64Type.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select 100000000000000, 200000000000000, 300000000000000.45 from rdb$database "
		"where 10000000000000 = cast(? as bigint)"};

	statement.setScaledInt64(0, ScaledInt64{10000000000000, 0});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getScaledInt64(0).value() == ScaledInt64{100000000000000, 0}));
	BOOST_CHECK((statement.getScaledInt64(1).value() == ScaledInt64{200000000000000, 0}));
	BOOST_CHECK((statement.getScaledInt64(2).value() == ScaledInt64{30000000000000045, -2}));

	statement.setScaledInt64(0, ScaledInt64{0, 0});
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getScaledInt64(0).has_value());

	statement.setScaledInt64(0, ScaledInt64{100000000000000, -1});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getScaledInt64(0).value() == ScaledInt64{100000000000000, 0}));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(scaledBoostInt128Type)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-scaledBoostInt128Type.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select 100000000000000000000, 200000000000000000000, 300000000000000000000.45 from rdb$database "
		"where 10000000000000000000 = cast(? as int128)"};

	statement.setScaledBoostInt128(0, ScaledBoostInt128{BoostInt128{"10000000000000000000"}, 0});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(
		(statement.getScaledBoostInt128(0).value() == ScaledBoostInt128{BoostInt128{"100000000000000000000"}, 0}));
	BOOST_CHECK(
		(statement.getScaledBoostInt128(1).value() == ScaledBoostInt128{BoostInt128{"200000000000000000000"}, 0}));
	BOOST_CHECK(
		(statement.getScaledBoostInt128(2).value() == ScaledBoostInt128{BoostInt128{"30000000000000000000045"}, -2}));

	statement.setScaledBoostInt128(0, ScaledBoostInt128{0, 0});
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getScaledBoostInt128(0).has_value());

	statement.setScaledBoostInt128(0, ScaledBoostInt128{BoostInt128{"100000000000000000000"}, -1});
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(
		(statement.getScaledBoostInt128(0).value() == ScaledBoostInt128{BoostInt128{"100000000000000000000"}, 0}));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(floatType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-floatType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select 1000, 200000, 300000.45, cast(400000.67 as float) from rdb$database "
		"where 10000000 = cast(? as float)"};

	statement.setFloat(0, 10000000.0f);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getFloat(0).value() == 1000.0f));
	BOOST_CHECK((statement.getFloat(1).value() == 200000.0f));
	BOOST_CHECK((statement.getFloat(2).value() == 300000.45f));
	BOOST_CHECK((statement.getFloat(3).value() == 400000.67f));

	statement.setFloat(0, 0.0f);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getFloat(0).has_value());

	statement.setFloat(0, 10000000.0f);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getFloat(0).value() == 1000.0f));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(doubleType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-doubleType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select 1000, 200000, 300000.45, cast(400000.67 as double precision) from rdb$database "
		"where 10000000 = cast(? as double precision)"};

	statement.setDouble(0, 10000000.0);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getDouble(0).value() == 1000.0));
	BOOST_CHECK((statement.getDouble(1).value() == 200000.0));
	BOOST_CHECK((statement.getDouble(2).value() == 300000.45));
	BOOST_CHECK((statement.getDouble(3).value() == 400000.67));

	statement.setDouble(0, 0.0);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getDouble(0).has_value());

	statement.setDouble(0, 10000000.0);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK((statement.getDouble(0).value() == 1000.0));
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "1000");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(dateType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-dateType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select date '2024-02-29', date '2023-12-31' from rdb$database "
		"where cast(? as date) = date '2024-02-29'"};

	const Date february29{std::chrono::year{2024}, std::chrono::month{2}, std::chrono::day{29}};
	const Date december31{std::chrono::year{2023}, std::chrono::month{12}, std::chrono::day{31}};

	statement.setDate(0, february29);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getDate(0).value() == february29);
	BOOST_CHECK(statement.getDate(1).value() == december31);
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "2024-02-29");

	statement.setString(0, "2024-02-29");
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getDate(0).value() == february29);

	statement.setDate(0, std::nullopt);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getDate(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(timeType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-timeType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select time '13:14:15.1234', time '23:59:59' from rdb$database "
		"where cast(? as time) = time '13:14:15.1234'"};

	const Time thirteenFourteen{std::chrono::hours{13} + std::chrono::minutes{14} + std::chrono::seconds{15} +
		std::chrono::microseconds{123400}};
	const Time almostMidnight{std::chrono::hours{23} + std::chrono::minutes{59} + std::chrono::seconds{59}};
	const auto thirteenFourteenDuration = thirteenFourteen.to_duration();
	const auto almostMidnightDuration = almostMidnight.to_duration();

	statement.setTime(0, thirteenFourteen);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getTime(0).value().to_duration() == thirteenFourteenDuration);
	BOOST_CHECK(statement.getTime(1).value().to_duration() == almostMidnightDuration);
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "13:14:15.1234");

	statement.setString(0, "13:14:15.1234");
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getTime(0).value().to_duration() == thirteenFourteenDuration);

	statement.setTime(0, std::nullopt);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getTime(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(timeTzType)
{
	Attachment attachment{CLIENT, getTempFile("Statement-timeTzType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select time '13:14:15.1234 UTC', "
		"       time '23:59:59 America/Sao_Paulo' "
		"from rdb$database "
		"where cast(? as time with time zone) = "
		"      time '13:14:15.1234 UTC'"};

	const auto normalizeDuration = [](std::chrono::microseconds duration)
	{
		const auto day = std::chrono::hours{24};

		duration %= day;

		if (duration < std::chrono::microseconds::zero())
			duration += day;

		return duration;
	};
	const auto makeTime = [](unsigned hours, unsigned minutes, unsigned seconds, unsigned fractions = 0)
	{
		return Time{std::chrono::hours{hours} + std::chrono::minutes{minutes} + std::chrono::seconds{seconds} +
			std::chrono::microseconds{static_cast<std::int64_t>(fractions) * 100}};
	};
	const auto makeTimeTz = [&](std::string_view zone, const Time& local)
	{
		if (zone == "UTC")
			return TimeTz{local, std::string{zone}};

		if (zone == "America/Sao_Paulo")
		{
			const auto utcDuration = normalizeDuration(local.to_duration() + std::chrono::hours{3});
			return TimeTz{Time{utcDuration}, std::string{zone}};
		}

		BOOST_FAIL("Unexpected time zone in test");
		return TimeTz{};
	};
	const auto toLocal = [&](const TimeTz& value)
	{
		if (value.zone == "UTC")
			return value.utcTime;

		if (value.zone == "America/Sao_Paulo")
		{
			const auto localDuration = normalizeDuration(value.utcTime.to_duration() - std::chrono::hours{3});
			return Time{localDuration};
		}

		BOOST_FAIL("Unexpected time zone in test");
		return value.utcTime;
	};

	const auto utcLocal = makeTime(13, 14, 15, 1234);
	const auto saoPauloLocal = makeTime(23, 59, 59);
	const auto utcTimeTz = makeTimeTz("UTC", utcLocal);
	const auto saoPauloTimeTz = makeTimeTz("America/Sao_Paulo", saoPauloLocal);

	statement.setTimeTz(0, utcTimeTz);
	BOOST_CHECK(statement.execute(transaction));

	const auto firstValue = statement.getTimeTz(0).value();
	const auto secondValue = statement.getTimeTz(1).value();

	BOOST_CHECK(firstValue.utcTime.to_duration() == utcTimeTz.utcTime.to_duration());
	BOOST_CHECK_EQUAL(firstValue.zone, utcTimeTz.zone);
	BOOST_CHECK(secondValue.utcTime.to_duration() == saoPauloTimeTz.utcTime.to_duration());
	BOOST_CHECK_EQUAL(secondValue.zone, saoPauloTimeTz.zone);
	BOOST_CHECK(toLocal(firstValue).to_duration() == utcLocal.to_duration());
	BOOST_CHECK(toLocal(secondValue).to_duration() == saoPauloLocal.to_duration());
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "13:14:15.1234 UTC");

	statement.setString(0, "13:14:15.1234 UTC");
	BOOST_CHECK(statement.execute(transaction));
	const auto stringValue = statement.getTimeTz(0).value();
	BOOST_CHECK(stringValue.utcTime.to_duration() == utcTimeTz.utcTime.to_duration());
	BOOST_CHECK_EQUAL(stringValue.zone, utcTimeTz.zone);
	BOOST_CHECK(toLocal(stringValue).to_duration() == utcLocal.to_duration());

	statement.setTimeTz(0, std::nullopt);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getTimeTz(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(timestampType)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-timestampType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select timestamp '2024-02-29 13:14:15.1234', timestamp '2023-12-31 23:59:59' from rdb$database "
		"where cast(? as timestamp) = timestamp '2024-02-29 13:14:15.1234'"};

	const auto february29Time = std::chrono::hours{13} + std::chrono::minutes{14} + std::chrono::seconds{15} +
		std::chrono::microseconds{123400};
	const Timestamp february29{Date{std::chrono::year{2024}, std::chrono::month{2}, std::chrono::day{29}},
		Time{std::chrono::duration_cast<std::chrono::microseconds>(february29Time)}};
	const auto december31Time =
		std::chrono::hours{23} + std::chrono::minutes{59} + std::chrono::seconds{59} + std::chrono::microseconds{0};
	const Timestamp december31{Date{std::chrono::year{2023}, std::chrono::month{12}, std::chrono::day{31}},
		Time{std::chrono::duration_cast<std::chrono::microseconds>(december31Time)}};

	statement.setTimestamp(0, february29);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getTimestamp(0).value() == february29);
	BOOST_CHECK(statement.getTimestamp(1).value() == december31);
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "2024-02-29 13:14:15.1234");

	statement.setString(0, "2024-02-29 13:14:15.1234");
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getTimestamp(0).value() == february29);

	statement.setTimestamp(0, std::nullopt);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getTimestamp(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(timestampTzType)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-timestampTzType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select timestamp '2024-02-29 13:14:15.1234 UTC', "
		"       timestamp '2023-12-31 23:59:59 America/Sao_Paulo' "
		"from rdb$database "
		"where cast(? as timestamp with time zone) = "
		"      timestamp '2024-02-29 13:14:15.1234 UTC'"};

	const auto toLocal = [](const TimestampTz& value)
	{
		if (value.zone == "UTC")
			return value.utcTimestamp;

		if (value.zone == "America/Sao_Paulo")
			return Timestamp::fromLocalTime(value.utcTimestamp.toLocalTime() - std::chrono::hours{3});

		BOOST_FAIL("Unexpected time zone in test");
		return value.utcTimestamp;
	};
	const auto makeTimestampTz = [](std::string_view zone, const Timestamp& local)
	{
		if (zone == "UTC")
			return TimestampTz{local, std::string{zone}};

		if (zone == "America/Sao_Paulo")
		{
			const auto utcLocal = local.toLocalTime() + std::chrono::hours{3};
			return TimestampTz{Timestamp::fromLocalTime(utcLocal), std::string{zone}};
		}

		BOOST_FAIL("Unexpected time zone in test");
		return TimestampTz{};
	};

	const auto utcLocalTime = std::chrono::hours{13} + std::chrono::minutes{14} + std::chrono::seconds{15} +
		std::chrono::microseconds{123400};
	const Timestamp utcLocal{Date{std::chrono::year{2024}, std::chrono::month{2}, std::chrono::day{29}},
		Time{std::chrono::duration_cast<std::chrono::microseconds>(utcLocalTime)}};
	const auto saoPauloLocalTime =
		std::chrono::hours{23} + std::chrono::minutes{59} + std::chrono::seconds{59} + std::chrono::microseconds{0};
	const Timestamp saoPauloLocal{Date{std::chrono::year{2023}, std::chrono::month{12}, std::chrono::day{31}},
		Time{std::chrono::duration_cast<std::chrono::microseconds>(saoPauloLocalTime)}};
	const auto utcTimestampTz = makeTimestampTz("UTC", utcLocal);
	const auto saoPauloTimestampTz = makeTimestampTz("America/Sao_Paulo", saoPauloLocal);

	statement.setTimestampTz(0, utcTimestampTz);
	BOOST_CHECK(statement.execute(transaction));

	const auto firstValue = statement.getTimestampTz(0).value();
	const auto secondValue = statement.getTimestampTz(1).value();

	BOOST_CHECK(firstValue.utcTimestamp == utcTimestampTz.utcTimestamp);
	BOOST_CHECK_EQUAL(firstValue.zone, utcTimestampTz.zone);
	BOOST_CHECK(secondValue.utcTimestamp == saoPauloTimestampTz.utcTimestamp);
	BOOST_CHECK_EQUAL(secondValue.zone, saoPauloTimestampTz.zone);
	BOOST_CHECK(toLocal(firstValue) == utcLocal);
	BOOST_CHECK(toLocal(secondValue) == saoPauloLocal);
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "2024-02-29 13:14:15.1234 UTC");

	statement.setString(0, "2024-02-29 13:14:15.1234 UTC");
	BOOST_CHECK(statement.execute(transaction));
	const auto stringValue = statement.getTimestampTz(0).value();
	BOOST_CHECK(stringValue.utcTimestamp == utcTimestampTz.utcTimestamp);
	BOOST_CHECK_EQUAL(stringValue.zone, utcTimestampTz.zone);
	BOOST_CHECK(toLocal(stringValue) == utcLocal);

	statement.setTimestampTz(0, std::nullopt);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getTimestampTz(0).has_value());

	transaction.commit();
}

// FIXME: remove isolated tests above

BOOST_AUTO_TEST_CASE(getters)
{
	Attachment attachment{CLIENT, getTempFile("Statement-getters.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select null,
		       true,
		       cast(1 as smallint),
		       cast(-32768 as smallint),
		       cast(32767 as smallint),
		       cast(2 as integer),
		       cast(200000 as integer),
		       cast(-2147483648 as integer),
		       cast(2147483647 as integer),
		       200.678,
		       cast(3 as bigint),
		       cast(300000000000 as bigint),
		       cast(-9223372036854775808 as bigint),
		       cast(9223372036854775807 as bigint),
		       300000000.678,
		       cast(4 as int128),
		       cast(400000000000000000000 as int128),
		       cast(-170141183460469231731687303715884105728 as int128),
		       cast(170141183460469231731687303715884105727 as int128),
		       400000000000000000.678,
		       cast(0.6 as numeric(4, 1)),
		       cast(-0.6 as numeric(4, 1)),
		       cast(0.6 as numeric(9, 1)),
		       cast(-0.6 as numeric(9, 1)),
		       cast(0.6 as numeric(18, 1)),
		       cast(-0.6 as numeric(18, 1)),
		       cast(0.6 as numeric(34, 1)),
		       cast(-0.6 as numeric(34, 1)),
		       cast(0.6 as decfloat(16)),
		       cast(-0.6 as decfloat(16)),
		       cast(0.6 as decfloat(34)),
		       cast(-0.6 as decfloat(34)),
		       cast(12345.67 as float),
		       cast(123456.789 as double precision),
		       _ascii 'abc',
		       _utf8 '12345'
		    from rdb$database
		)"""};

	BOOST_CHECK(statement.execute(transaction));

	unsigned index = 0;

	// null: null
	BOOST_CHECK(statement.isNull(index));
	BOOST_CHECK(!statement.getBool(index).has_value());
	BOOST_CHECK(!statement.getInt16(index).has_value());
	BOOST_CHECK(!statement.getInt32(index).has_value());
	BOOST_CHECK(!statement.getInt64(index).has_value());
	BOOST_CHECK(!statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK(!statement.getBoostInt128(index).has_value());
	BOOST_CHECK(!statement.getScaledInt16(index).has_value());
	BOOST_CHECK(!statement.getScaledInt32(index).has_value());
	BOOST_CHECK(!statement.getScaledInt64(index).has_value());
	BOOST_CHECK(!statement.getScaledBoostInt128(index).has_value());
	BOOST_CHECK(!statement.getFloat(index).has_value());
	BOOST_CHECK(!statement.getDouble(index).has_value());
	BOOST_CHECK(!statement.getOpaqueDecFloat16(index).has_value());
	BOOST_CHECK(!statement.getOpaqueDecFloat34(index).has_value());
	BOOST_CHECK(!statement.getBoostDecFloat16(index).has_value());
	BOOST_CHECK(!statement.getBoostDecFloat34(index).has_value());
	BOOST_CHECK(!statement.getString(index).has_value());

	// boolean: true
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK(statement.getBool(index).value());
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getFloat(index), FbCppException);
	BOOST_CHECK_THROW(statement.getDouble(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "true");

	// smallint: cast(1 as smallint)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{1, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{1, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{1, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{1}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 1.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 1.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"1"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"1"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "1");

	// smallint min: cast(-32768 as smallint)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), -32'768);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -32'768);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -32'768);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-32'768});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{-32'768, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{-32'768, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{-32'768, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{-32'768}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -32'768.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -32'768.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-32768"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-32768"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-32768");

	// smallint max: cast(32767 as smallint)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 32'767);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 32'767);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 32'767);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{32'767});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{32'767, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{32'767, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{32'767, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{32'767}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 32'767.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 32'767.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"32767"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"32767"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "32767");

	// integer that fits in smallint: cast(2 as integer)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 2);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 2);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 2);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{2});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{2, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{2, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{2, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{2}, 0}));
	BOOST_CHECK_EQUAL((statement.getFloat(index).value()), 2.0f);
	BOOST_CHECK_EQUAL((statement.getDouble(index).value()), 2.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"2"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"2"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "2");

	// integer that does not fit in smallint: cast(200000 as integer)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 200'000);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 200'000);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{200'000});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{200'000, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{200'000, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{200'000}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 200'000.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 200'000.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"200000"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"200000"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "200000");

	// integer min: cast(-2147483648 as integer)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -2'147'483'648);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -2'147'483'648);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-2'147'483'648});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{-2'147'483'648, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{-2'147'483'648, 0}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{-2'147'483'648}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -2'147'483'648.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -2'147'483'648.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-2147483648"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-2147483648"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-2147483648");

	// integer max: cast(2147483647 as integer)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 2'147'483'647);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 2'147'483'647);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{2'147'483'647});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{2'147'483'647, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{2'147'483'647, 0}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{2'147'483'647}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 2'147'483'647.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 2'147'483'647.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"2147483647"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"2147483647"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "2147483647");

	// numeric(6,3): 200.678
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 201);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 201);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 201);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{201});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{200'678, -3}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{200'678, -3}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{200'678}, -3}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 200.678f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 200.678);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"200.678"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"200.678"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "200.678");

	// bigint that fits in smallint: cast(3 as bigint)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 3);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 3);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 3);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{3});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{3, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{3, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{3, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{3}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 3.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 3.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"3"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"3"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "3");

	// bigint that does not fit in integer: cast(300000000000 as bigint)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 300'000'000'000);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{300'000'000'000});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{300'000'000'000, 0}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{300'000'000'000}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 300'000'000'000.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 300'000'000'000.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"300000000000"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"300000000000"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "300000000000");

	// bigint min: cast(-9223372036854775808 as bigint)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -9223372036854775807LL - 1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-9223372036854775807LL - 1});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{-9223372036854775807LL - 1, 0}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{-9223372036854775807LL - 1}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), static_cast<float>(-9223372036854775807LL - 1));
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), static_cast<double>(-9223372036854775807LL - 1));
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-9223372036854775808"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-9223372036854775808"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-9223372036854775808");

	// bigint max: cast(9223372036854775807 as bigint)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 9223372036854775807LL);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{9223372036854775807LL});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{9223372036854775807LL, 0}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{9223372036854775807LL}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), static_cast<float>(9223372036854775807LL));
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), static_cast<double>(9223372036854775807LL));
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"9223372036854775807"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"9223372036854775807"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "9223372036854775807");

	// numeric(12,3): 300000000.678
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 300'000'001);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 300'000'001);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{300'000'001});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{300'000'000'678, -3}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{300'000'000'678}, -3}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 300'000'000.678f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 300'000'000.678);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"300000000.678"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"300000000.678"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "300000000.678");

	// int128 that fits in smallint: cast(4 as int128)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 4);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 4);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 4);
	BOOST_CHECK(statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{4});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{4, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{4, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{4, 0}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{4}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 4.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 4.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"4"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"4"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "4");

	// int128 that does not fit in bigint: cast(400000000000000000000 as int128)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt64(index), FbCppException);
	BOOST_CHECK(statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{"400000000000000000000"});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{"400000000000000000000"}, 0}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 400'000'000'000'000'000'000.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 400'000'000'000'000'000'000.0);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"400000000000000000000"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"400000000000000000000"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "400000000000000000000");

	// int128 min: cast(-170141183460469231731687303715884105728 as int128)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt64(index), FbCppException);
	BOOST_CHECK(statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{"-170141183460469231731687303715884105728"});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(),
		(ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, 0}));
	BOOST_CHECK_EQUAL(
		statement.getFloat(index).value(), static_cast<float>(BoostInt128{"-170141183460469231731687303715884105728"}));
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(),
		static_cast<double>(BoostInt128{"-170141183460469231731687303715884105728"}));
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(
		statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-170141183460469231731687303715884105728"});
	BOOST_CHECK_EQUAL(
		statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-170141183460469231731687303715884105728"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-170141183460469231731687303715884105728");

	// int128 max: cast(170141183460469231731687303715884105727 as int128)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt64(index), FbCppException);
	BOOST_CHECK(statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{"170141183460469231731687303715884105727"});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(),
		(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, 0}));
	BOOST_CHECK_EQUAL(
		statement.getFloat(index).value(), static_cast<float>(BoostInt128{"170141183460469231731687303715884105727"}));
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(),
		static_cast<double>(BoostInt128{"170141183460469231731687303715884105727"}));
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(
		statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"170141183460469231731687303715884105727"});
	BOOST_CHECK_EQUAL(
		statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"170141183460469231731687303715884105727"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "170141183460469231731687303715884105727");

	// numeric(21,3): 400000000000000000.678
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 400000000000000001);
	BOOST_CHECK(statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{"400000000000000001"});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{"400000000000000000678"}, -3}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 400'000'000'000'000'000.678f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 400'000'000'000'000'000.678);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"400000000000000000.678"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"400000000000000000.678"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "400000000000000000.678");

	// numeric(4,1): 0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "0.6");

	// numeric(4,1): -0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{-6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-0.6");

	// numeric(9,1): 0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "0.6");

	// numeric(9,1): -0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{-6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-0.6");

	// numeric(18,1): 0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "0.6");

	// numeric(18,1): -0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{-6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-0.6");

	// numeric(34,1): 0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 1);
	BOOST_CHECK(statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "0.6");

	// numeric(34,1): -0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -1);
	BOOST_CHECK(statement.getScaledOpaqueInt128(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index).value(), (ScaledInt16{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index).value(), (ScaledInt32{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index).value(), (ScaledInt64{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index).value(), (ScaledBoostInt128{BoostInt128{-6}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-0.6");

	// decfloat(16): 0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{1});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 0.6);
	BOOST_CHECK(statement.getOpaqueDecFloat16(index).has_value());
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"0.6"});
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "0.6");

	// decfloat(16): -0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-1});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK(statement.getOpaqueDecFloat16(index).has_value());
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-0.6");

	// decfloat(34): 0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{1});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK(statement.getOpaqueDecFloat34(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"0.6"});
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 0.6);
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "0.6");

	// decfloat(34): -0.6
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), -1);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), -1);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{-1});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), -0.6f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK(statement.getOpaqueDecFloat34(index).has_value());
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"-0.6"});
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), -0.6);
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "-0.6");

	// float: cast(12345.67 as float)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt16(index).value(), 12346);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 12346);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 12346);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{12346});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_CLOSE(statement.getFloat(index).value(), 12345.67f, 0.00001);
	BOOST_CHECK_CLOSE(statement.getDouble(index).value(), 12345.67, 0.00001);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK(
		boost::multiprecision::abs(statement.getBoostDecFloat16(index).value() - BoostDecFloat16{"12345.67"}) < 0.0001);
	BOOST_CHECK(
		boost::multiprecision::abs(statement.getBoostDecFloat34(index).value() - BoostDecFloat34{"12345.67"}) < 0.0001);
	BOOST_CHECK(statement.getString(index).value().starts_with("12345.6"));

	// double precision: cast(123456.789 as double precision)
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getInt32(index).value(), 123457);
	BOOST_CHECK_EQUAL(statement.getInt64(index).value(), 123457);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index).value(), BoostInt128{123457});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getFloat(index).value(), 123456.789f);
	BOOST_CHECK_EQUAL(statement.getDouble(index).value(), 123456.789);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index).value(), BoostDecFloat16{"123456.789"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index).value(), BoostDecFloat34{"123456.789"});
	BOOST_CHECK(statement.getString(index).value().starts_with("123456.789"));

	// alpha ascii string: _ascii 'abc'
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getFloat(index), FbCppException);
	BOOST_CHECK_THROW(statement.getDouble(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "abc");

	// numeric utf8 string: _utf8 '12345'
	++index;
	BOOST_CHECK(!statement.isNull(index));
	BOOST_CHECK_THROW(statement.getBool(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledOpaqueInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt32(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledInt64(index), FbCppException);
	BOOST_CHECK_THROW(statement.getScaledBoostInt128(index), FbCppException);
	BOOST_CHECK_THROW(statement.getFloat(index), FbCppException);
	BOOST_CHECK_THROW(statement.getDouble(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getOpaqueDecFloat34(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostDecFloat16(index), FbCppException);
	BOOST_CHECK_THROW(statement.getBoostDecFloat34(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getString(index).value(), "12345");

	transaction.commit();
}

// FIXME: setters test

BOOST_AUTO_TEST_CASE(setNull)
{
	Attachment attachment{CLIENT, getTempFile("Statement-setNull.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select cast(? as boolean),
		       cast(? as smallint)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setNull(index++);
	statement.setNull(index++);
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK(statement.isNull(index++));
	BOOST_CHECK(statement.isNull(index++));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setBool)
{
	Attachment attachment{CLIENT, getTempFile("Statement-setBool.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select cast(? as boolean),
		       cast(? as boolean)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setBool(index++, true);
	statement.setBool(index++, false);
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK(statement.getBool(index++).value());
	BOOST_CHECK(!statement.getBool(index++).value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setInt16)
{
	Attachment attachment{CLIENT, getTempFile("Statement-setInt16.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setInt16(index++, -32768);
	statement.setInt16(index++, 32767);
	statement.setInt16(index++, -32768);
	statement.setInt16(index++, 32767);
	statement.setInt16(index++, -32768);
	statement.setInt16(index++, 32767);
	statement.setInt16(index++, -32768);
	statement.setInt16(index++, 32767);
	statement.setInt16(index++, -32768);
	statement.setInt16(index++, 32767);
	statement.setInt16(index++, -32768);
	statement.setInt16(index++, 32767);
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 32767);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), BoostInt128{32767});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{327670, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index++).value(), (ScaledBoostInt128{BoostInt128{327670}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-32768.0"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"32767.0"});
	BOOST_CHECK_EQUAL(statement.getFloat(index++).value(), -32768.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index++).value(), 32767.0);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setScaledInt16)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-setScaledInt16.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setScaledInt16(index++, ScaledInt16{-32768, -1});
	statement.setScaledInt16(index++, ScaledInt16{32767, -1});
	statement.setScaledInt16(index++, ScaledInt16{-32768, -1});
	statement.setScaledInt16(index++, ScaledInt16{32767, -1});
	statement.setScaledInt16(index++, ScaledInt16{-32768, -1});
	statement.setScaledInt16(index++, ScaledInt16{32767, -1});
	statement.setScaledInt16(index++, ScaledInt16{-32768, -1});
	statement.setScaledInt16(index++, ScaledInt16{32767, -1});
	statement.setScaledInt16(index++, ScaledInt16{-32768, -1});
	statement.setScaledInt16(index++, ScaledInt16{32767, -1});
	statement.setScaledInt16(index++, ScaledInt16{-32768, -1});
	statement.setScaledInt16(index++, ScaledInt16{32767, -1});
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -3277);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 3277);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), -3277);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), BoostInt128{3277});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index++).value(), (ScaledInt16{-32768, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{32767, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-32768, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index++).value(), (ScaledBoostInt128{BoostInt128{32767}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-3276.8"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"3276.7"});
	BOOST_CHECK_CLOSE(statement.getFloat(index++).value(), -3276.8f, 0.001);
	BOOST_CHECK_CLOSE(statement.getDouble(index++).value(), 3276.7, 0.0000001);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setInt32)
{
	Attachment attachment{CLIENT, getTempFile("Statement-setInt32.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setInt32(index++, -32768);
	statement.setInt32(index++, 2147483647);
	statement.setInt32(index++, -2147483647 - 1);
	statement.setInt32(index++, 2147483647);
	statement.setInt32(index++, -32768);
	statement.setInt32(index++, 214748364);
	statement.setInt32(index++, -2147483647 - 1);
	statement.setInt32(index++, 2147483647);
	statement.setInt32(index++, -2147483647 - 1);
	statement.setInt32(index++, 2147483647);
	statement.setInt32(index++, -2147483647 - 1);
	statement.setInt32(index++, 2147483647);
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 2147483647);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), -2147483648LL);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), BoostInt128{2147483647});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{2147483640, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-21474836480LL, -1}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index++).value(), (ScaledBoostInt128{BoostInt128{21474836470LL}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-2147483648.0"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"2147483647.0"});
	BOOST_CHECK_EQUAL(statement.getFloat(index++).value(), -2147483648.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index++).value(), 2147483647.0);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setScaledInt32)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-setScaledInt32.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setScaledInt32(index++, ScaledInt32{-327680, -1});
	statement.setScaledInt32(index++, ScaledInt32{2147483647, 0});
	statement.setScaledInt32(index++, ScaledInt32{-2147483647 - 1, 0});
	statement.setScaledInt32(index++, ScaledInt32{2147483647, 0});
	statement.setScaledInt32(index++, ScaledInt32{-327680, -1});
	statement.setScaledInt32(index++, ScaledInt32{214748364, -1});
	statement.setScaledInt32(index++, ScaledInt32{-2147483647 - 1, -1});
	statement.setScaledInt32(index++, ScaledInt32{2147483647, -1});
	statement.setScaledInt32(index++, ScaledInt32{-2147483647 - 1, 0});
	statement.setScaledInt32(index++, ScaledInt32{2147483647, 0});
	statement.setScaledInt32(index++, ScaledInt32{-2147483647 - 1, 0});
	statement.setScaledInt32(index++, ScaledInt32{2147483647, 0});
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 2147483647);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), -2147483648LL);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), BoostInt128{2147483647});
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{214748364, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-2147483648, -1}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index++).value(), (ScaledBoostInt128{BoostInt128{2147483647}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-2147483648.0"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"2147483647.0"});
	BOOST_CHECK_EQUAL(statement.getFloat(index++).value(), -2147483648.0f);
	BOOST_CHECK_EQUAL(statement.getDouble(index++).value(), 2147483647.0);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setInt64)
{
	Attachment attachment{CLIENT, getTempFile("Statement-setInt64.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	constexpr auto int64Min = -9223372036854775807LL - 1;
	constexpr auto int64Max = 9223372036854775807LL;

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setInt64(index++, -32768);
	statement.setInt64(index++, 2147483647);
	statement.setInt64(index++, int64Min);
	statement.setInt64(index++, int64Max);
	statement.setInt64(index++, -32768);
	statement.setInt64(index++, 214748364);
	statement.setInt64(index++, -9223372036854775LL);
	statement.setInt64(index++, int64Max);
	statement.setInt64(index++, -9223372036854775LL);
	statement.setInt64(index++, int64Max);
	statement.setInt64(index++, -9223372036854775LL);
	statement.setInt64(index++, int64Max);
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 2147483647);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), int64Min);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), BoostInt128{int64Max});
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{2147483640LL, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-92233720368547750LL, -1}));
	BOOST_CHECK_EQUAL(
		statement.getScaledBoostInt128(index++).value(), (ScaledBoostInt128{BoostInt128{"92233720368547758070"}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-9223372036854775.0"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"9223372036854775807.0"});
	BOOST_CHECK_CLOSE(statement.getFloat(index++).value(), static_cast<float>(-9223372036854775LL), 0.001);
	BOOST_CHECK_CLOSE(statement.getDouble(index++).value(), static_cast<double>(int64Max), 1e-7);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setScaledInt64)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-setScaledInt64.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	constexpr auto int64Min = -9223372036854775807LL - 1;
	constexpr auto int64Max = 9223372036854775807LL;

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setScaledInt64(index++, ScaledInt64{-327680, -1});
	statement.setScaledInt64(index++, ScaledInt64{2147483647, 0});
	statement.setScaledInt64(index++, ScaledInt64{int64Min, 0});
	statement.setScaledInt64(index++, ScaledInt64{int64Max, 0});
	statement.setScaledInt64(index++, ScaledInt64{-327680, -1});
	statement.setScaledInt64(index++, ScaledInt64{2147483640LL, -1});
	statement.setScaledInt64(index++, ScaledInt64{-922337203685477580, -1});
	statement.setScaledInt64(index++, ScaledInt64{int64Max, -1});
	statement.setScaledInt64(index++, ScaledInt64{-9223372036854775LL, 0});
	statement.setScaledInt64(index++, ScaledInt64{int64Max, 0});
	statement.setScaledInt64(index++, ScaledInt64{-9223372036854775LL, 0});
	statement.setScaledInt64(index++, ScaledInt64{int64Max, 0});
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 2147483647);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), int64Min);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), BoostInt128{int64Max});
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{2147483640LL, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-922337203685477580, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index++).value(), (ScaledBoostInt128{BoostInt128{int64Max}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-9223372036854775.0"});
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"9223372036854775807.0"});
	BOOST_CHECK_CLOSE(statement.getFloat(index++).value(), static_cast<float>(-9223372036854775LL), 0.001);
	BOOST_CHECK_CLOSE(statement.getDouble(index++).value(), static_cast<double>(int64Max), 1e-7);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setBoostInt128)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-setBoostInt128.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const BoostInt128 int128Min{"-170141183460469231731687303715884105728"};
	const BoostInt128 int128Max{"170141183460469231731687303715884105727"};
	const BoostInt128 bigInt128Scaled{"1234567890123456789012345678900"};
	const BoostInt128 bigInt128{"123456789012345678901234567890"};

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setBoostInt128(index++, BoostInt128{-32768});
	statement.setBoostInt128(index++, BoostInt128{2147483647});
	statement.setBoostInt128(index++, BoostInt128{-9223372036854775807LL - 1});
	statement.setBoostInt128(index++, int128Min);
	statement.setBoostInt128(index++, BoostInt128{-32768});
	statement.setBoostInt128(index++, BoostInt128{123456789});
	statement.setBoostInt128(index++, BoostInt128{-12345678901234567LL});
	statement.setBoostInt128(index++, bigInt128);
	statement.setBoostInt128(index++, BoostInt128{-1234567890123456});
	statement.setBoostInt128(index++, bigInt128);
	statement.setBoostInt128(index++, BoostInt128{-9223372036854775807LL - 1});
	statement.setBoostInt128(index++, int128Max);
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 2147483647);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), -9223372036854775807LL - 1);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), int128Min);
	BOOST_CHECK_THROW(statement.getScaledInt16(index), FbCppException);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{1234567890LL, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-123456789012345670LL, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index++).value(),
		(ScaledBoostInt128{BoostInt128{"1234567890123456789012345678900"}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-1234567890123456.0"});
	BOOST_CHECK_EQUAL(
		statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"123456789012345678901234567890.0"});
	BOOST_CHECK_CLOSE(statement.getFloat(index++).value(), static_cast<float>(-9223372036854775807LL - 1), 0.001);
	BOOST_CHECK_CLOSE(statement.getDouble(index++).value(), static_cast<double>(int128Max), 1e-7);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setScaledBoostInt128)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-setScaledBoostInt128.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const BoostInt128 bigInt128{"123456789012345678901234567890"};
	const BoostInt128 bigInt128Scaled{"1234567890123456789012345678900"};
	const BoostInt128 int128Min{"-170141183460469231731687303715884105728"};
	const BoostInt128 int128Max{"170141183460469231731687303715884105727"};

	Statement statement{attachment, transaction, R"""(
		select cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(6,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as decfloat(16)),
		       cast(? as decfloat(34)),
		       cast(? as float),
		       cast(? as double precision)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{-327680}, -1});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{2147483647}, 0});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{-9223372036854775807LL - 1}, 0});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{int128Min, 0});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{-327680}, -1});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{1234567890}, -1});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{-123456789012345670}, -1});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{bigInt128Scaled, -1});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{-1234567890123456}, 0});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{bigInt128, 0});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{BoostInt128{-9223372036854775807LL - 1}, 0});
	statement.setScaledBoostInt128(index++, ScaledBoostInt128{int128Max, 0});
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), -32768);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 2147483647);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), -9223372036854775807LL - 1);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), int128Min);
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-327680, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{1234567890LL, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{-123456789012345670LL, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index++).value(),
		(ScaledBoostInt128{BoostInt128{"1234567890123456789012345678900"}, -1}));
	BOOST_CHECK_EQUAL(statement.getBoostDecFloat16(index++).value(), BoostDecFloat16{"-1234567890123456.0"});
	BOOST_CHECK_EQUAL(
		statement.getBoostDecFloat34(index++).value(), BoostDecFloat34{"123456789012345678901234567890.0"});
	BOOST_CHECK_CLOSE(statement.getFloat(index++).value(), static_cast<float>(-9223372036854775807LL - 1), 0.001);
	BOOST_CHECK_CLOSE(statement.getDouble(index++).value(), static_cast<double>(int128Max), 1e-7);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(setString)
{
	Attachment attachment{CLIENT, getTempFile("Statement-setString.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction, R"""(
		select cast(? as boolean),
		       cast(? as smallint),
		       cast(? as integer),
		       cast(? as bigint),
		       cast(? as int128),
		       cast(? as numeric(4,1)),
		       cast(? as numeric(9,1)),
		       cast(? as numeric(18,1)),
		       cast(? as numeric(34,1)),
		       cast(? as float),
		       cast(? as double precision),
		       cast(? as varchar(5) character set ascii),
		       cast(? as char(10) character set utf8)
		    from rdb$database
		)"""};

	unsigned index = 0;

	statement.setString(index++, "true");
	statement.setString(index++, "1");
	statement.setString(index++, "1");
	statement.setString(index++, "1");
	statement.setString(index++, "1");
	statement.setString(index++, "0.6");
	statement.setString(index++, "-0.6");
	statement.setString(index++, "0.67");
	statement.setString(index++, "-0.67");
	statement.setString(index++, "0.78");
	statement.setString(index++, "-0.78");
	BOOST_CHECK_THROW(statement.setString(index, "123456"), FbCppException);
	statement.setString(index++, "abc");
	statement.setString(index++, "defgh");
	BOOST_CHECK(statement.execute(transaction));
	index = 0;
	BOOST_CHECK(statement.getBool(index++));
	BOOST_CHECK_EQUAL(statement.getInt16(index++).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt32(index++).value(), 1);
	BOOST_CHECK_EQUAL(statement.getInt64(index++).value(), 1);
	BOOST_CHECK_EQUAL(statement.getBoostInt128(index++).value(), BoostInt128{1});
	BOOST_CHECK_EQUAL(statement.getScaledInt16(index++).value(), (ScaledInt16{6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt32(index++).value(), (ScaledInt32{-6, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledInt64(index++).value(), (ScaledInt64{7, -1}));
	BOOST_CHECK_EQUAL(statement.getScaledBoostInt128(index++).value(), (ScaledBoostInt128{BoostInt128{-7}, -1}));
	BOOST_CHECK_EQUAL(statement.getFloat(index++).value(), 0.78f);
	BOOST_CHECK_EQUAL(statement.getDouble(index++).value(), -0.78);
	BOOST_CHECK_EQUAL(statement.getString(index++).value(), "abc");
	BOOST_CHECK_EQUAL(statement.getString(index++).value(), "defgh     ");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(opaqueDateType)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-opaqueDateType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select date '2024-02-29' from rdb$database "
		"where cast(? as date) = date '2024-02-29'"};

	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const auto opaqueDate =
		converter.dateToOpaqueDate(Date{std::chrono::year{2024}, std::chrono::month{2}, std::chrono::day{29}});
	statement.setOpaqueDate(0, opaqueDate);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getOpaqueDate(0) == opaqueDate);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(opaqueTimeType)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-opaqueTimeType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select time '12:34:56.7891' from rdb$database "
		"where cast(? as time) = time '12:34:56.7891'"};

	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const Time time{std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} +
		std::chrono::microseconds{789100}};
	const auto opaqueTime = converter.timeToOpaqueTime(time);
	statement.setOpaqueTime(0, opaqueTime);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getOpaqueTime(0).value() == opaqueTime);
	BOOST_CHECK(statement.getTime(0).value().to_duration() == time.to_duration());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(opaqueTimeTzType)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-opaqueTimeTzType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select time '13:14:15.1234 America/Sao_Paulo' from rdb$database "
		"where cast(? as time with time zone) = "
		"      time '13:14:15.1234 America/Sao_Paulo'"};

	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const auto timeTz = converter.stringToTimeTz("13:14:15.1234 America/Sao_Paulo");
	const auto opaqueTimeTz = converter.timeTzToOpaqueTimeTz(timeTz);

	statement.setOpaqueTimeTz(0, opaqueTimeTz);
	BOOST_CHECK(statement.execute(transaction));

	const auto fetchedOpaqueTimeTz = statement.getOpaqueTimeTz(0).value();
	const auto roundTripTimeTz = converter.opaqueTimeTzToTimeTz(fetchedOpaqueTimeTz);
	BOOST_CHECK(roundTripTimeTz.utcTime.to_duration() == timeTz.utcTime.to_duration());
	BOOST_CHECK_EQUAL(roundTripTimeTz.zone, timeTz.zone);

	const auto fetchedTimeTz = statement.getTimeTz(0).value();
	BOOST_CHECK(fetchedTimeTz.utcTime.to_duration() == timeTz.utcTime.to_duration());
	BOOST_CHECK_EQUAL(fetchedTimeTz.zone, timeTz.zone);
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "13:14:15.1234 America/Sao_Paulo");

	statement.setOpaqueTimeTz(0, std::nullopt);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getOpaqueTimeTz(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(opaqueTimestampType)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-opaqueTimestampType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select timestamp '2024-02-29 12:34:56.7891' from rdb$database "
		"where cast(? as timestamp) = timestamp '2024-02-29 12:34:56.7891'"};

	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const Date date{std::chrono::year{2024}, std::chrono::month{2}, std::chrono::day{29}};
	const auto timeOfDay = std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} +
		std::chrono::microseconds{789100};
	const Timestamp timestamp{date, Time{std::chrono::duration_cast<std::chrono::microseconds>(timeOfDay)}};
	const auto opaqueTimestamp = converter.timestampToOpaqueTimestamp(timestamp);
	statement.setOpaqueTimestamp(0, opaqueTimestamp);
	BOOST_CHECK(statement.execute(transaction));
	BOOST_CHECK(statement.getOpaqueTimestamp(0).value() == opaqueTimestamp);
	BOOST_CHECK(statement.getTimestamp(0).value() == timestamp);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(opaqueTimestampTzType)
{
	Attachment attachment{
		CLIENT, getTempFile("Statement-opaqueTimestampTzType.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement statement{attachment, transaction,
		"select timestamp '2024-02-29 12:34:56.7891 America/Sao_Paulo' from rdb$database "
		"where cast(? as timestamp with time zone) = "
		"      timestamp '2024-02-29 12:34:56.7891 America/Sao_Paulo'"};

	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::CalendarConverter converter{CLIENT, &statusWrapper};

	const auto timestampTz = converter.stringToTimestampTz("2024-02-29 12:34:56.7891 America/Sao_Paulo");
	const auto opaqueTimestampTz = converter.timestampTzToOpaqueTimestampTz(timestampTz);

	statement.setOpaqueTimestampTz(0, opaqueTimestampTz);
	BOOST_CHECK(statement.execute(transaction));

	const auto fetchedOpaqueTimestampTz = statement.getOpaqueTimestampTz(0).value();
	const auto roundTripTimestampTz = converter.opaqueTimestampTzToTimestampTz(fetchedOpaqueTimestampTz);
	BOOST_CHECK(roundTripTimestampTz.utcTimestamp.toLocalTime() == timestampTz.utcTimestamp.toLocalTime());
	BOOST_CHECK_EQUAL(roundTripTimestampTz.zone, timestampTz.zone);

	const auto fetchedTimestampTz = statement.getTimestampTz(0).value();
	BOOST_CHECK(fetchedTimestampTz.utcTimestamp.toLocalTime() == timestampTz.utcTimestamp.toLocalTime());
	BOOST_CHECK_EQUAL(fetchedTimestampTz.zone, timestampTz.zone);
	BOOST_CHECK_EQUAL(statement.getString(0).value(), "2024-02-29 12:34:56.7891 America/Sao_Paulo");

	statement.setOpaqueTimestampTz(0, std::nullopt);
	BOOST_CHECK(!statement.execute(transaction));
	BOOST_CHECK(!statement.getOpaqueTimestampTz(0).has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_SUITE_END()
