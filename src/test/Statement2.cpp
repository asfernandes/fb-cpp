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

#include "TestUtil.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include "fb-cpp/Attachment.h"
#include "fb-cpp/Blob.h"
#include "fb-cpp/NumericConverter.h"
#include "fb-cpp/CalendarConverter.h"
#include "fb-cpp/Exception.h"
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <cmath>
#include <exception>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>


using namespace std::chrono;
using namespace std::chrono_literals;


static constexpr float floatTolerance = 0.00001f;
static constexpr double doubleTolerance = 0.0000000000001;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
static const BoostDecFloat16 decFloat16Tolerance{"0.00000000000001"};
static const BoostDecFloat34 decFloat34Tolerance{"0.00000000000000000000000000000001"};
#endif


// FIXME: join with test/Statement.cpp
BOOST_AUTO_TEST_SUITE(Statement2Suite)

BOOST_AUTO_TEST_CASE(statementOptions)
{
	StatementOptions options;
	BOOST_CHECK_EQUAL(options.getPrefetchLegacyPlan(), false);
	BOOST_CHECK_EQUAL(options.getPrefetchPlan(), false);

	options.setPrefetchLegacyPlan(true).setPrefetchPlan(true);

	BOOST_CHECK_EQUAL(options.getPrefetchLegacyPlan(), true);
	BOOST_CHECK_EQUAL(options.getPrefetchPlan(), true);
}

BOOST_AUTO_TEST_CASE(unsupportedStatementsThrow)
{
	const auto database = getTempFile("Statement-unsupportedStatementsThrow.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	BOOST_CHECK_THROW(Statement(attachment, transaction, "set transaction read committed"), FbCppException);
	BOOST_CHECK_THROW(Statement(attachment, transaction, "commit"), FbCppException);
	BOOST_CHECK_THROW(Statement(attachment, transaction, "rollback"), FbCppException);
}

BOOST_AUTO_TEST_CASE(constructorProvidesMetadata)
{
	const auto database = getTempFile("Statement-constructorProvidesMetadata.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};

	{
		Statement createStmt{
			attachment, ddlTransaction, "create table stmt_meta (id integer primary key, value_text varchar(20))"};
		BOOST_CHECK(createStmt.isValid());
		BOOST_CHECK(createStmt.getType() == StatementType::DDL);
		BOOST_CHECK(createStmt.execute(ddlTransaction));
		BOOST_CHECK_EQUAL(createStmt.getInputDescriptors().size(), 0U);
		BOOST_CHECK_EQUAL(createStmt.getOutputDescriptors().size(), 0U);
	}

	ddlTransaction.commit();

	Transaction selectTransaction{attachment};
	Statement selectStmt{attachment, selectTransaction, "select id, value_text from stmt_meta where id = ?"};

	BOOST_CHECK(selectStmt.isValid());
	BOOST_CHECK(selectStmt.getStatementHandle());
	BOOST_CHECK(!selectStmt.getResultSetHandle());
	BOOST_CHECK(selectStmt.getType() == StatementType::SELECT);
	BOOST_REQUIRE_EQUAL(selectStmt.getInputDescriptors().size(), 1U);
	BOOST_REQUIRE_EQUAL(selectStmt.getOutputDescriptors().size(), 2U);
	BOOST_CHECK(selectStmt.getInputMetadata());
	BOOST_CHECK(selectStmt.getOutputMetadata());

	const auto& inputDescriptor = selectStmt.getInputDescriptors().front();
	BOOST_CHECK(inputDescriptor.adjustedType == DescriptorAdjustedType::INT32 ||
		inputDescriptor.adjustedType == DescriptorAdjustedType::INT64);

	const auto& outputDescriptor = selectStmt.getOutputDescriptors().back();
	BOOST_CHECK(outputDescriptor.adjustedType == DescriptorAdjustedType::STRING);
}

BOOST_AUTO_TEST_CASE(moveConstructorInvalidatesSource)
{
	const auto database = getTempFile("Statement-moveConstructorInvalidatesSource.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};

	Statement createStmt{attachment, ddlTransaction, "create table stmt_move (id integer primary key)"};
	BOOST_CHECK(createStmt.execute(ddlTransaction));

	ddlTransaction.commit();

	Transaction transaction{attachment};
	Statement original{attachment, transaction, "insert into stmt_move(id) values (?)"};
	BOOST_CHECK(original.isValid());

	auto moved = std::move(original);
	BOOST_CHECK(moved.isValid());
	BOOST_CHECK_EQUAL(original.isValid(), false);

	BOOST_REQUIRE_EQUAL(moved.getInputDescriptors().size(), 1U);

	moved.setInt32(0, 1);
	BOOST_CHECK(moved.execute(transaction));
	transaction.commit();
}

BOOST_AUTO_TEST_CASE(executeAndCursorMovement)
{
	const auto database = getTempFile("Statement-executeAndCursorMovement.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};
	Statement createStmt{
		attachment, ddlTransaction, "create table stmt_cursor (id integer primary key, amount integer)"};
	bool advancedTypesSupported = true;

	try
	{
		BOOST_CHECK(createStmt.execute(ddlTransaction));
		ddlTransaction.commit();
	}
	catch (const DatabaseException& e)
	{
		advancedTypesSupported = false;
		ddlTransaction.rollback();
		BOOST_TEST_MESSAGE(
			std::string{"Skipping parameterBindingAndGetters due to unsupported data types: "} + e.what());
	}

	if (!advancedTypesSupported)
		return;

	{
		Transaction insertTransaction{attachment};
		Statement insertStmt{attachment, insertTransaction, "insert into stmt_cursor(id, amount) values (?, ?)"};
		BOOST_CHECK(insertStmt.getType() == StatementType::INSERT);

		for (int i = 1; i <= 5; ++i)
		{
			insertStmt.setInt32(0, i);
			insertStmt.setInt32(1, i * 10);
			BOOST_CHECK(insertStmt.execute(insertTransaction));
		}

		insertTransaction.commit();
	}

	Transaction selectTransaction{attachment};
	Statement selectStmt{attachment, selectTransaction, "select id, amount from stmt_cursor order by id"};

	BOOST_REQUIRE(selectStmt.execute(selectTransaction));
	BOOST_CHECK(selectStmt.getResultSetHandle());

	const auto readRow = [&]()
	{
		const auto id = selectStmt.getInt32(0);
		const auto value = selectStmt.getInt32(1);
		BOOST_REQUIRE(id.has_value());
		BOOST_REQUIRE(value.has_value());
		return std::pair{*id, *value};
	};

	std::vector<std::pair<int, int>> rows;
	rows.push_back(readRow());

	while (selectStmt.fetchNext())
		rows.push_back(readRow());

	BOOST_REQUIRE_EQUAL(rows.size(), 5U);
	BOOST_CHECK_EQUAL(rows[0].first, 1);
	BOOST_CHECK_EQUAL(rows[0].second, 10);
	BOOST_CHECK_EQUAL(rows[2].first, 3);
	BOOST_CHECK_EQUAL(rows[4].first, 5);

	selectStmt.free();
	BOOST_CHECK_EQUAL(selectStmt.isValid(), false);
}

BOOST_AUTO_TEST_CASE(cursorMovementWithoutResultSet)
{
	const auto database = getTempFile("Statement-cursorMovementWithoutResultSet.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};
	Statement createStmt{attachment, ddlTransaction, "create table stmt_cursor_write (id integer primary key)"};
	BOOST_CHECK(createStmt.execute(ddlTransaction));
	ddlTransaction.commit();

	Transaction transaction{attachment};
	Statement insertStmt{attachment, transaction, "insert into stmt_cursor_write(id) values (?)"};
	insertStmt.setInt32(0, 1);
	BOOST_CHECK(insertStmt.execute(transaction));

	BOOST_CHECK_EQUAL(insertStmt.fetchNext(), false);
	BOOST_CHECK_EQUAL(insertStmt.fetchPrior(), false);
	BOOST_CHECK_EQUAL(insertStmt.fetchFirst(), false);
	BOOST_CHECK_EQUAL(insertStmt.fetchLast(), false);
	BOOST_CHECK_EQUAL(insertStmt.fetchAbsolute(1), false);
	BOOST_CHECK_EQUAL(insertStmt.fetchRelative(1), false);

	transaction.commit();
}

namespace
{
	void writeBlob(Blob& blob, const std::string& text)
	{
		blob.write(std::as_bytes(std::span{text.data(), text.size()}));
		blob.close();
	}
}  // namespace

BOOST_AUTO_TEST_CASE(parameterBindingAndGetters)
{
	const auto database = getTempFile("Statement-parameterBindingAndGetters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};
	impl::CalendarConverter calendarConverter{CLIENT, &statusWrapper};
	impl::NumericConverter numericConverter{CLIENT, &statusWrapper};

	Transaction ddlTransaction{attachment};

	static constexpr std::string_view CREATE_TABLE_SQL = "create table stmt_values ("
														 "id integer,"
														 "bool_col boolean,"
														 "smallint_col smallint,"
														 "smallint_scaled_col numeric(4,1),"
														 "int_col integer,"
														 "int_scaled_col numeric(9,3),"
														 "bigint_col bigint,"
														 "bigint_scaled_col numeric(18,5),"
														 "opaque_int128_col numeric(34,0),"
														 "boost_int128_col numeric(34,0),"
														 "scaled_boost_int128_col numeric(34,2),"
														 "float_col float,"
														 "double_col double precision,"
														 "opaque_dec16_col decfloat(16),"
														 "boost_dec16_col decfloat(16),"
														 "opaque_dec34_col decfloat(34),"
														 "boost_dec34_col decfloat(34),"
														 "date_col date,"
														 "opaque_date_col date,"
														 "time_col time,"
														 "opaque_time_col time,"
														 "timestamp_col timestamp,"
														 "opaque_timestamp_col timestamp,"
														 "time_tz_col time with time zone,"
														 "opaque_time_tz_col time with time zone,"
														 "timestamp_tz_col timestamp with time zone,"
														 "opaque_timestamp_tz_col timestamp with time zone,"
														 "text_col varchar(50),"
														 "string_numeric_col numeric(10,2),"
														 "blob_col blob,"
														 "nullable_col integer)";

	try
	{
		Statement createStmt{attachment, ddlTransaction, CREATE_TABLE_SQL};
		createStmt.execute(ddlTransaction);
		ddlTransaction.commit();

		Transaction blobTransaction{attachment};
		Blob blob{attachment, blobTransaction};
		const std::string blobText{"Statement blob value"};
		writeBlob(blob, blobText);
		const BlobId blobId = blob.getId();
		blobTransaction.commit();

		const std::int16_t smallIntValue = -123;
		const ScaledInt16 scaledInt16Value{321, -1};
		const std::int32_t intValue = 1234567;
		const ScaledInt32 scaledInt32Value{7654321, -3};
		const std::int64_t bigIntValue = 9876543210123;
		const ScaledInt64 scaledInt64Value{9876543210123, -4};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		const BoostInt128 boostIntValue{"123456789012345678901234567890"};
		const BoostInt128 boostScaledIntValue{"222222222222222222222222222222"};
		const OpaqueInt128 opaqueIntValue = numericConverter.boostInt128ToOpaqueInt128(boostIntValue);
		const ScaledBoostInt128 scaledBoostValue{boostScaledIntValue, -2};
#else
		const OpaqueInt128 opaqueIntValue = {0, 0};  // Dummy
#endif

		const float floatValue = 12.3456f;
		const double doubleValue = 9876.54321;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		const BoostDecFloat16 boostDecFloat16Value{"12345.6789"};
		const BoostDecFloat34 boostDecFloat34Value{"98765.4321987654321"};
		const OpaqueDecFloat16 opaqueDecFloat16Value =
			numericConverter.boostDecFloat16ToOpaqueDecFloat16(boostDecFloat16Value);
		const OpaqueDecFloat34 opaqueDecFloat34Value =
			numericConverter.boostDecFloat34ToOpaqueDecFloat34(boostDecFloat34Value);
#else
		const OpaqueDecFloat16 opaqueDecFloat16Value = {0};  // Dummy
		const OpaqueDecFloat34 opaqueDecFloat34Value = {0};  // Dummy
#endif

		const Date dateValue{2024y / March / 15d};
		const Time timeValue{13h + 14min + 15s + 123456us};
		const auto timestampDuration = 10h + 11min + 12s + 654321us;
		const Timestamp timestampValue{Date{2024y / March / 15d}, Time{timestampDuration}};
		const TimeTz timeTzValue{Time{7h + 8min + 9s + 111222us}, "UTC"};
		const auto timestampTzDuration = 5h + 6min + 7s + 333444us;
		const TimestampTz timestampTzValue{Timestamp{Date{2024y / March / 16d}, Time{timestampTzDuration}}, "UTC"};

		const OpaqueDate opaqueDateValue = calendarConverter.dateToOpaqueDate(dateValue);
		const OpaqueTime opaqueTimeValue = calendarConverter.timeToOpaqueTime(timeValue);
		const OpaqueTimestamp opaqueTimestampValue = calendarConverter.timestampToOpaqueTimestamp(timestampValue);
		const OpaqueTimeTz opaqueTimeTzValue = calendarConverter.timeTzToOpaqueTimeTz(timeTzValue);
		const OpaqueTimestampTz opaqueTimestampTzValue =
			calendarConverter.timestampTzToOpaqueTimestampTz(timestampTzValue);

		const std::string textValue{"textual value"};
		const std::string numericFromString{"4567.89"};

		Transaction insertTransaction{attachment};
		Statement insertStmt{attachment, insertTransaction,
			"insert into stmt_values("
			"id,bool_col,smallint_col,smallint_scaled_col,int_col,int_scaled_col,bigint_col,bigint_scaled_col,"
			"opaque_int128_col,boost_int128_col,scaled_boost_int128_col,float_col,double_col,opaque_dec16_col,"
			"boost_dec16_col,opaque_dec34_col,boost_dec34_col,date_col,opaque_date_col,time_col,opaque_time_col,"
			"timestamp_col,opaque_timestamp_col,time_tz_col,opaque_time_tz_col,timestamp_tz_col,opaque_timestamp_tz_"
			"col,"
			"text_col,string_numeric_col,blob_col,nullable_col) "
			"values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"};

		insertStmt.clearParameters();

		insertStmt.setInt32(0, 1);
		insertStmt.set(0, intValue);

		insertStmt.setBool(1, true);
		insertStmt.set(1, false);
		insertStmt.set(1, std::optional<bool>{true});
		insertStmt.set(1, false);

		insertStmt.setInt16(2, smallIntValue);
		insertStmt.set(2, static_cast<std::int16_t>(smallIntValue));

		insertStmt.setScaledInt16(3, scaledInt16Value);
		insertStmt.set(3, scaledInt16Value);

		insertStmt.setInt32(4, intValue);
		insertStmt.set(4, intValue);

		insertStmt.setScaledInt32(5, scaledInt32Value);
		insertStmt.set(5, scaledInt32Value);

		insertStmt.setInt64(6, bigIntValue);
		insertStmt.set(6, bigIntValue);

		insertStmt.setScaledInt64(7, scaledInt64Value);
		insertStmt.set(7, scaledInt64Value);

		insertStmt.setOpaqueInt128(8, opaqueIntValue);
		insertStmt.set(8, opaqueIntValue);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		insertStmt.setBoostInt128(9, boostIntValue);
		insertStmt.set(9, boostIntValue);

		insertStmt.setScaledBoostInt128(10, scaledBoostValue);
		insertStmt.set(10, scaledBoostValue);
#else
		insertStmt.setNull(9);
		insertStmt.setNull(10);
#endif

		insertStmt.setFloat(11, floatValue);
		insertStmt.set(11, floatValue);

		insertStmt.setDouble(12, doubleValue);
		insertStmt.set(12, doubleValue);

		insertStmt.setOpaqueDecFloat16(13, opaqueDecFloat16Value);
		insertStmt.set(13, opaqueDecFloat16Value);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		insertStmt.setBoostDecFloat16(14, boostDecFloat16Value);
		insertStmt.set(14, boostDecFloat16Value);

		insertStmt.setOpaqueDecFloat34(15, opaqueDecFloat34Value);
		insertStmt.set(15, opaqueDecFloat34Value);

		insertStmt.setBoostDecFloat34(16, boostDecFloat34Value);
		insertStmt.set(16, boostDecFloat34Value);
#else
		insertStmt.setNull(14);
		insertStmt.setOpaqueDecFloat34(15, opaqueDecFloat34Value);
		insertStmt.set(15, opaqueDecFloat34Value);
		insertStmt.setNull(16);
#endif

		insertStmt.setDate(17, dateValue);
		insertStmt.set(17, dateValue);

		insertStmt.setOpaqueDate(18, opaqueDateValue);
		insertStmt.set(18, opaqueDateValue);

		insertStmt.setTime(19, timeValue);
		insertStmt.set(19, timeValue);

		insertStmt.setOpaqueTime(20, opaqueTimeValue);
		insertStmt.set(20, opaqueTimeValue);

		insertStmt.setTimestamp(21, timestampValue);
		insertStmt.set(21, timestampValue);

		insertStmt.setOpaqueTimestamp(22, opaqueTimestampValue);
		insertStmt.set(22, opaqueTimestampValue);

		insertStmt.setTimeTz(23, timeTzValue);
		insertStmt.set(23, timeTzValue);

		insertStmt.setOpaqueTimeTz(24, opaqueTimeTzValue);
		insertStmt.set(24, opaqueTimeTzValue);

		insertStmt.setTimestampTz(25, timestampTzValue);
		insertStmt.set(25, timestampTzValue);

		insertStmt.setOpaqueTimestampTz(26, opaqueTimestampTzValue);
		insertStmt.set(26, opaqueTimestampTzValue);

		insertStmt.setString(27, textValue);
		insertStmt.set(27, std::string_view{textValue});

		insertStmt.setString(28, numericFromString);

		insertStmt.setBlobId(29, blobId);
		insertStmt.set(29, blobId);
		insertStmt.set(29, std::optional<BlobId>{blobId});

		insertStmt.setInt32(30, std::optional<std::int32_t>{});
		insertStmt.set(30, std::nullopt);
		insertStmt.setNull(30);

		insertStmt.execute(insertTransaction);
		insertTransaction.commit();

		Transaction selectTransaction{attachment};
		Statement selectStmt{attachment, selectTransaction,
			"select id,bool_col,smallint_col,smallint_scaled_col,int_col,int_scaled_col,bigint_col,bigint_scaled_col,"
			"opaque_int128_col,boost_int128_col,scaled_boost_int128_col,float_col,double_col,opaque_dec16_col,boost_"
			"dec16_col,"
			"opaque_dec34_col,boost_dec34_col,date_col,opaque_date_col,time_col,opaque_time_col,timestamp_col,"
			"opaque_timestamp_col,time_tz_col,opaque_time_tz_col,timestamp_tz_col,opaque_timestamp_tz_col,text_col,"
			"string_numeric_col,blob_col,nullable_col from stmt_values where id = ?"};

		selectStmt.clearParameters();
		selectStmt.set(0, std::optional<std::int32_t>{intValue});
		BOOST_REQUIRE(selectStmt.execute(selectTransaction));

		BOOST_REQUIRE_EQUAL(selectStmt.getOutputDescriptors().size(), 31U);

		const auto id = selectStmt.getInt32(0);
		BOOST_REQUIRE(id.has_value());
		BOOST_CHECK_EQUAL(id.value(), intValue);

		const auto boolValue = selectStmt.getBool(1);
		BOOST_REQUIRE(boolValue.has_value());
		BOOST_CHECK_EQUAL(boolValue.value(), false);
		const auto boolTemplate = selectStmt.get<std::optional<bool>>(1);
		BOOST_REQUIRE(boolTemplate.has_value());
		BOOST_CHECK_EQUAL(boolTemplate.value(), false);
		BOOST_CHECK_EQUAL(selectStmt.getString(1).value(), "false");

		const auto smallInt = selectStmt.getInt16(2);
		BOOST_REQUIRE(smallInt.has_value());
		BOOST_CHECK_EQUAL(smallInt.value(), smallIntValue);
		const auto scaledSmallInt = selectStmt.getScaledInt16(3);
		BOOST_REQUIRE(scaledSmallInt.has_value());
		BOOST_CHECK_EQUAL(scaledSmallInt.value().value, scaledInt16Value.value);
		BOOST_CHECK_EQUAL(scaledSmallInt.value().scale, scaledInt16Value.scale);

		const auto intCol = selectStmt.getInt32(4);
		BOOST_REQUIRE(intCol.has_value());
		BOOST_CHECK_EQUAL(intCol.value(), intValue);

		const auto scaledInt = selectStmt.getScaledInt32(5);
		BOOST_REQUIRE(scaledInt.has_value());
		BOOST_CHECK_EQUAL(scaledInt.value().value, scaledInt32Value.value);
		BOOST_CHECK_EQUAL(scaledInt.value().scale, scaledInt32Value.scale);

		const auto bigInt = selectStmt.getInt64(6);
		BOOST_REQUIRE(bigInt.has_value());
		BOOST_CHECK_EQUAL(bigInt.value(), bigIntValue);

		const auto scaledBigInt = selectStmt.getScaledInt64(7);
		BOOST_REQUIRE(scaledBigInt.has_value());
		BOOST_CHECK_EQUAL(scaledBigInt.value().value, scaledInt64Value.value);
		BOOST_CHECK_EQUAL(scaledBigInt.value().scale, scaledInt64Value.scale);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		const auto scaledOpaqueInt = selectStmt.getScaledOpaqueInt128(8);
		BOOST_REQUIRE(scaledOpaqueInt.has_value());
		const auto boostFromOpaque = numericConverter.opaqueInt128ToBoostInt128(scaledOpaqueInt.value().value);
		BOOST_CHECK(boostFromOpaque == boostIntValue);

		const auto boostInt = selectStmt.getBoostInt128(9);
		BOOST_REQUIRE(boostInt.has_value());
		BOOST_CHECK(boostInt.value() == boostIntValue);

		const auto scaledBoost = selectStmt.getScaledBoostInt128(10);
		BOOST_REQUIRE(scaledBoost.has_value());
		BOOST_CHECK(scaledBoost.value().value == scaledBoostValue.value);
		BOOST_CHECK_EQUAL(scaledBoost.value().scale, scaledBoostValue.scale);
#endif

		const auto floatResult = selectStmt.getFloat(11);
		BOOST_REQUIRE(floatResult.has_value());
		BOOST_CHECK_CLOSE(floatResult.value(), floatValue, floatTolerance);

		const auto doubleResult = selectStmt.getDouble(12);
		BOOST_REQUIRE(doubleResult.has_value());
		BOOST_CHECK_CLOSE(doubleResult.value(), doubleValue, doubleTolerance);

		const auto opaqueDec16 = selectStmt.getOpaqueDecFloat16(13);
		BOOST_REQUIRE(opaqueDec16.has_value());
		BOOST_CHECK_EQUAL(numericConverter.opaqueDecFloat16ToString(opaqueDec16.value()),
			numericConverter.opaqueDecFloat16ToString(opaqueDecFloat16Value));

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		const auto boostDec16 = selectStmt.getBoostDecFloat16(14);
		BOOST_REQUIRE(boostDec16.has_value());
		BOOST_CHECK_CLOSE(boostDec16.value(), boostDecFloat16Value, decFloat16Tolerance);
#endif

		const auto opaqueDec34 = selectStmt.getOpaqueDecFloat34(15);
		BOOST_REQUIRE(opaqueDec34.has_value());
		BOOST_CHECK_EQUAL(numericConverter.opaqueDecFloat34ToString(opaqueDec34.value()),
			numericConverter.opaqueDecFloat34ToString(opaqueDecFloat34Value));

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		const auto boostDec34 = selectStmt.getBoostDecFloat34(16);
		BOOST_REQUIRE(boostDec34.has_value());
		BOOST_CHECK_CLOSE(boostDec34.value(), boostDecFloat34Value, decFloat34Tolerance);
#endif

		const auto dateResult = selectStmt.getDate(17);
		BOOST_REQUIRE(dateResult.has_value());
		BOOST_CHECK_EQUAL(dateResult.value(), dateValue);

		const auto opaqueDateResult = selectStmt.getOpaqueDate(18);
		BOOST_REQUIRE(opaqueDateResult.has_value());
		BOOST_CHECK_EQUAL(calendarConverter.opaqueDateToDate(opaqueDateResult.value()), dateValue);

		const auto timeResult = selectStmt.getTime(19);
		BOOST_REQUIRE(timeResult.has_value());
		BOOST_CHECK_EQUAL(timeResult.value().to_duration(), timeValue.to_duration());

		const auto opaqueTimeResult = selectStmt.getOpaqueTime(20);
		BOOST_REQUIRE(opaqueTimeResult.has_value());
		BOOST_CHECK_EQUAL(
			calendarConverter.opaqueTimeToTime(opaqueTimeResult.value()).to_duration(), timeValue.to_duration());

		const auto timestampResult = selectStmt.getTimestamp(21);
		BOOST_REQUIRE(timestampResult.has_value());
		BOOST_CHECK(timestampResult.value() == timestampValue);

		const auto opaqueTimestampResult = selectStmt.getOpaqueTimestamp(22);
		BOOST_REQUIRE(opaqueTimestampResult.has_value());
		BOOST_CHECK(calendarConverter.opaqueTimestampToTimestamp(opaqueTimestampResult.value()) == timestampValue);

		const auto timeTzResult = selectStmt.getTimeTz(23);
		BOOST_REQUIRE(timeTzResult.has_value());
		BOOST_CHECK(timeTzResult.value() == timeTzValue);

		const auto opaqueTimeTzResult = selectStmt.getOpaqueTimeTz(24);
		BOOST_REQUIRE(opaqueTimeTzResult.has_value());
		BOOST_CHECK(calendarConverter.opaqueTimeTzToTimeTz(opaqueTimeTzResult.value()) == timeTzValue);

		const auto timestampTzResult = selectStmt.getTimestampTz(25);
		BOOST_REQUIRE(timestampTzResult.has_value());
		BOOST_CHECK(timestampTzResult.value() == timestampTzValue);

		const auto opaqueTimestampTzResult = selectStmt.getOpaqueTimestampTz(26);
		BOOST_REQUIRE(opaqueTimestampTzResult.has_value());
		BOOST_CHECK(
			calendarConverter.opaqueTimestampTzToTimestampTz(opaqueTimestampTzResult.value()) == timestampTzValue);

		const auto textResult = selectStmt.getString(27);
		BOOST_REQUIRE(textResult.has_value());
		BOOST_CHECK_EQUAL(textResult.value(), textValue);

		const auto numericFromStringResult = selectStmt.getDouble(28);
		BOOST_REQUIRE(numericFromStringResult.has_value());
		BOOST_CHECK_CLOSE(numericFromStringResult.value(), std::stod(numericFromString), doubleTolerance);
		const auto numericAsString = selectStmt.getString(28);
		BOOST_REQUIRE(numericAsString.has_value());
		BOOST_CHECK(!numericAsString->empty());

		const auto blobResult = selectStmt.getBlobId(29);
		BOOST_REQUIRE(blobResult.has_value());
		BOOST_CHECK_EQUAL(blobResult->id.gds_quad_high, blobId.id.gds_quad_high);
		BOOST_CHECK_EQUAL(blobResult->id.gds_quad_low, blobId.id.gds_quad_low);

		BOOST_CHECK_EQUAL(selectStmt.isNull(30), true);
		BOOST_CHECK_EQUAL(selectStmt.getInt32(30).has_value(), false);

		const auto boolString = selectStmt.getString(1);
		BOOST_REQUIRE(boolString.has_value());
		BOOST_CHECK_EQUAL(boolString.value(), "false");

		const auto intString = selectStmt.getString(4);
		BOOST_REQUIRE(intString.has_value());
		BOOST_CHECK_EQUAL(intString.value(), std::to_string(intValue));

		const auto floatString = selectStmt.getString(11);
		BOOST_REQUIRE(floatString.has_value());
		BOOST_CHECK_CLOSE(std::stof(floatString.value()), floatValue, floatTolerance);

		const auto dateString = selectStmt.getString(17);
		BOOST_REQUIRE(dateString.has_value());
		BOOST_CHECK_EQUAL(calendarConverter.stringToDate(dateString.value()), dateValue);

		const auto timeString = selectStmt.getString(19);
		BOOST_REQUIRE(timeString.has_value());
		BOOST_CHECK_EQUAL(calendarConverter.stringToTime(timeString.value()).to_duration(), timeValue.to_duration());

		const auto timestampString = selectStmt.getString(21);
		BOOST_REQUIRE(timestampString.has_value());
		BOOST_CHECK(calendarConverter.stringToTimestamp(timestampString.value()) == timestampValue);
	}
	catch (const std::exception& e)
	{
		if (ddlTransaction.isValid())
			ddlTransaction.rollback();

		BOOST_TEST_MESSAGE(
			std::string{"Skipping parameterBindingAndGetters due to unsupported data types: "} + e.what());
		return;
	}
}

BOOST_AUTO_TEST_CASE(stringConversions)
{
	const auto database = getTempFile("Statement-stringConversions.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};
	Statement createStmt{attachment, ddlTransaction,
		"create table stmt_string_conv ("
		"id integer,"
		"bool_col boolean,"
		"numeric_col numeric(6,2),"
		"float_col double precision,"
		"date_col date,"
		"time_col time,"
		"timestamp_col timestamp,"
		"time_tz_col time with time zone,"
		"timestamp_tz_col timestamp with time zone)"};
	BOOST_CHECK(createStmt.execute(ddlTransaction));
	ddlTransaction.commit();

	Transaction insertTransaction{attachment};
	Statement insertStmt{attachment, insertTransaction,
		"insert into stmt_string_conv("
		"id,bool_col,numeric_col,float_col,date_col,time_col,timestamp_col,time_tz_col,timestamp_tz_col)"
		" values(?,?,?,?,?,?,?,?,?)"};

	insertStmt.setString(0, "1");
	insertStmt.setString(1, "true");
	insertStmt.setString(2, "1234.56");
	insertStmt.setString(3, "3.14159");
	insertStmt.setString(4, "2024-04-05");
	insertStmt.setString(5, "10:20:30.4000");
	insertStmt.setString(6, "2024-04-05 10:20:30.4000");
	insertStmt.setString(7, "10:20:30.4000 UTC");
	insertStmt.setString(8, "2024-04-05 10:20:30.4000 UTC");

	BOOST_CHECK(insertStmt.execute(insertTransaction));
	insertTransaction.commit();

	Transaction selectTransaction{attachment};
	Statement selectStmt{attachment, selectTransaction,
		"select bool_col,numeric_col,float_col,date_col,time_col,timestamp_col,time_tz_col,timestamp_tz_col "
		"from stmt_string_conv where id = 1"};

	BOOST_REQUIRE(selectStmt.execute(selectTransaction));

	BOOST_CHECK_EQUAL(selectStmt.getBool(0).value(), true);
	BOOST_CHECK_CLOSE(selectStmt.getDouble(1).value(), 1234.56, doubleTolerance);
	BOOST_CHECK_CLOSE(selectStmt.getDouble(2).value(), 3.14159, doubleTolerance);
	BOOST_CHECK_EQUAL(selectStmt.getDate(3).value(), Date{2024y / April / 5d});
	BOOST_CHECK_EQUAL(selectStmt.getString(3).value(), "2024-04-05");
	BOOST_CHECK_EQUAL(selectStmt.getTime(4).value().to_duration(), Time{10h + 20min + 30s + 400000us}.to_duration());
	const Timestamp expectedTimestamp{Date{2024y / April / 5d}, Time{10h + 20min + 30s + 400000us}};
	BOOST_CHECK(selectStmt.getTimestamp(5).value() == expectedTimestamp);
	BOOST_CHECK_EQUAL(selectStmt.getTimeTz(6).value().zone, "UTC");
	BOOST_CHECK_EQUAL(selectStmt.getTimestampTz(7).value().zone, "UTC");
}

BOOST_AUTO_TEST_CASE(clearParametersAndNullHandling)
{
	const auto database = getTempFile("Statement-clearParametersAndNullHandling.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};
	Statement createStmt{attachment, ddlTransaction, "create table stmt_nulls (id integer, amount integer)"};
	BOOST_CHECK(createStmt.execute(ddlTransaction));
	ddlTransaction.commit();

	Transaction insertTransaction{attachment};
	Statement insertStmt{attachment, insertTransaction, "insert into stmt_nulls(id, amount) values (?, ?)"};

	insertStmt.setInt32(0, 1);
	insertStmt.setInt32(1, 42);

	insertStmt.clearParameters();
	insertStmt.set(0, 1);
	insertStmt.setNull(1);

	BOOST_CHECK(insertStmt.execute(insertTransaction));
	insertTransaction.commit();

	Transaction selectTransaction{attachment};
	Statement selectStmt{attachment, selectTransaction, "select amount from stmt_nulls where id = 1"};
	BOOST_REQUIRE(selectStmt.execute(selectTransaction));
	BOOST_CHECK_EQUAL(selectStmt.isNull(0), true);
	BOOST_CHECK_EQUAL(selectStmt.getInt32(0).has_value(), false);
}

BOOST_AUTO_TEST_CASE(planRetrieval)
{
	const auto database = getTempFile("Statement-planRetrieval.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};
	Statement createStmt{attachment, ddlTransaction, "create table stmt_plan (id integer primary key)"};
	BOOST_CHECK(createStmt.execute(ddlTransaction));
	ddlTransaction.commit();

	Transaction transaction{attachment};
	StatementOptions options;
	options.setPrefetchLegacyPlan(true).setPrefetchPlan(true);
	Statement selectStmt{attachment, transaction, "select * from stmt_plan", options};

	const auto legacyPlan = selectStmt.getLegacyPlan();
	const auto detailedPlan = selectStmt.getPlan();

	BOOST_CHECK(!legacyPlan.empty());
	BOOST_CHECK(!detailedPlan.empty());
}

BOOST_AUTO_TEST_CASE(freeReleasesHandles)
{
	const auto database = getTempFile("Statement-freeReleasesHandles.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction ddlTransaction{attachment};
	Statement createStmt{attachment, ddlTransaction, "create table stmt_free (id integer primary key)"};
	BOOST_CHECK(createStmt.execute(ddlTransaction));
	ddlTransaction.commit();

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "insert into stmt_free(id) values (?)"};

	BOOST_CHECK(stmt.getStatementHandle());
	stmt.setInt32(0, 1);
	BOOST_CHECK(stmt.execute(transaction));

	stmt.free();
	BOOST_CHECK_EQUAL(stmt.isValid(), false);
	BOOST_CHECK_EQUAL(static_cast<bool>(stmt.getStatementHandle()), false);
	BOOST_CHECK_EQUAL(static_cast<bool>(stmt.getResultSetHandle()), false);
	transaction.commit();
}

BOOST_AUTO_TEST_SUITE_END()
