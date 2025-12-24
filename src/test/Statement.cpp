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
#include "fb-cpp/Blob.h"
#include "fb-cpp/Exception.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <chrono>
#include <cmath>
#include <limits>


BOOST_AUTO_TEST_SUITE(StatementLifecycleSuite)

BOOST_AUTO_TEST_CASE(constructorPreparesStatement)
{
	const auto database = getTempFile("Statement-constructorPreparesStatement.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 1 from rdb$database"};

	BOOST_CHECK(stmt.isValid());
}

BOOST_AUTO_TEST_CASE(moveConstructorTransfersOwnership)
{
	const auto database = getTempFile("Statement-moveConstructorTransfersOwnership.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt1{attachment, transaction, "select 1 from rdb$database"};
	BOOST_CHECK(stmt1.isValid());

	Statement stmt2{std::move(stmt1)};
	BOOST_CHECK(stmt2.isValid());
	BOOST_CHECK(!stmt1.isValid());
}

BOOST_AUTO_TEST_CASE(isNotValidAfterMove)
{
	const auto database = getTempFile("Statement-isNotValidAfterMove.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt1{attachment, transaction, "select 1 from rdb$database"};
	auto stmt2 = std::move(stmt1);

	BOOST_CHECK_EQUAL(stmt1.isValid(), false);
	BOOST_CHECK_EQUAL(stmt2.isValid(), true);
}

BOOST_AUTO_TEST_CASE(freeReleasesHandle)
{
	const auto database = getTempFile("Statement-freeReleasesHandle.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 1 from rdb$database"};
	BOOST_CHECK(stmt.isValid());
	BOOST_CHECK(stmt.getStatementHandle());

	stmt.free();
	BOOST_CHECK_EQUAL(stmt.isValid(), false);
	BOOST_CHECK_EQUAL(static_cast<bool>(stmt.getStatementHandle()), false);
	BOOST_CHECK_EQUAL(static_cast<bool>(stmt.getResultSetHandle()), false);
}

BOOST_AUTO_TEST_CASE(unsupportedStatementsThrow)
{
	const auto database = getTempFile("Statement-unsupportedStatementsThrow.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	BOOST_CHECK_THROW(Statement(attachment, transaction, "set transaction read committed"), FbCppException);
	BOOST_CHECK_THROW(Statement(attachment, transaction, "commit"), FbCppException);
	BOOST_CHECK_THROW(Statement(attachment, transaction, "rollback"), FbCppException);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementMetadataSuite)

BOOST_AUTO_TEST_CASE(getTypeReturnsCorrectStatementType)
{
	const auto database = getTempFile("Statement-getTypeReturnsCorrectStatementType.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddlStmt{attachment, transaction, "create table t (col integer)"};
	BOOST_CHECK(ddlStmt.getType() == StatementType::DDL);

	ddlStmt.execute(transaction);
	transaction.commitRetaining();

	Statement selectStmt{attachment, transaction, "select col from t"};
	BOOST_CHECK(selectStmt.getType() == StatementType::SELECT);

	Statement insertStmt{attachment, transaction, "insert into t (col) values (1)"};
	BOOST_CHECK(insertStmt.getType() == StatementType::INSERT);

	Statement updateStmt{attachment, transaction, "update t set col = 1"};
	BOOST_CHECK(updateStmt.getType() == StatementType::UPDATE);

	Statement deleteStmt{attachment, transaction, "delete from t"};
	BOOST_CHECK(deleteStmt.getType() == StatementType::DELETE);
}

BOOST_AUTO_TEST_CASE(getInputOutputDescriptors)
{
	const auto database = getTempFile("Statement-getInputOutputDescriptors.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction,
		"select cast(? as integer), cast(? as varchar(50)), cast(1.0 as double precision) from rdb$database"};

	BOOST_CHECK_EQUAL(stmt.getInputDescriptors().size(), 2U);
	BOOST_CHECK_EQUAL(stmt.getOutputDescriptors().size(), 3U);
}

BOOST_AUTO_TEST_CASE(constructorProvidesMetadataHandles)
{
	const auto database = getTempFile("Statement-constructorProvidesMetadataHandles.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{
		attachment, transaction, "select cast(? as integer), cast('val' as varchar(20)) from rdb$database"};
	BOOST_CHECK(select.isValid());
	BOOST_CHECK(select.getStatementHandle());
	BOOST_CHECK(!select.getResultSetHandle());
	BOOST_CHECK(select.getType() == StatementType::SELECT);
	BOOST_REQUIRE_EQUAL(select.getInputDescriptors().size(), 1U);
	BOOST_REQUIRE_EQUAL(select.getOutputDescriptors().size(), 2U);
	BOOST_CHECK(select.getInputMetadata());
	BOOST_CHECK(select.getOutputMetadata());

	const auto& inputDescriptor = select.getInputDescriptors().front();
	BOOST_CHECK(inputDescriptor.adjustedType == DescriptorAdjustedType::INT32 ||
		inputDescriptor.adjustedType == DescriptorAdjustedType::INT64);

	const auto& outputDescriptor = select.getOutputDescriptors().back();
	BOOST_CHECK(outputDescriptor.adjustedType == DescriptorAdjustedType::STRING);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementPlanSuite)

BOOST_AUTO_TEST_CASE(getLegacyPlan)
{
	const auto database = getTempFile("Statement-getLegacyPlan.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{
		attachment, transaction, "select 1 from rdb$database", StatementOptions().setPrefetchLegacyPlan(true)};

	const auto plan = stmt.getLegacyPlan();
	BOOST_CHECK(plan == "\nPLAN (RDB$DATABASE NATURAL)" || plan == "\nPLAN (\"SYSTEM\".\"RDB$DATABASE\" NATURAL)");
}

BOOST_AUTO_TEST_CASE(getPlan)
{
	const auto database = getTempFile("Statement-getPlan.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select 1 from rdb$database", StatementOptions().setPrefetchPlan(true)};

	const auto plan = stmt.getPlan();
	BOOST_CHECK(plan == "\nSelect Expression\n    -> Table \"RDB$DATABASE\" Full Scan" ||
		plan == "\nSelect Expression\n    -> Table \"SYSTEM\".\"RDB$DATABASE\" Full Scan");
}

BOOST_AUTO_TEST_CASE(statementOptionsGettersSetters)
{
	StatementOptions options;
	BOOST_CHECK_EQUAL(options.getPrefetchLegacyPlan(), false);
	BOOST_CHECK_EQUAL(options.getPrefetchPlan(), false);

	options.setPrefetchLegacyPlan(true).setPrefetchPlan(true);

	BOOST_CHECK_EQUAL(options.getPrefetchLegacyPlan(), true);
	BOOST_CHECK_EQUAL(options.getPrefetchPlan(), true);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementNullSuite)

BOOST_AUTO_TEST_CASE(setNullParameter)
{
	const auto database = getTempFile("Statement-setNullParameter.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as integer) from rdb$database"};
	select.setNull(0);
	select.execute(transaction);
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK(select.isNull(0));
	BOOST_CHECK(!select.getInt32(0).has_value());
}

BOOST_AUTO_TEST_CASE(clearParametersToNull)
{
	const auto database = getTempFile("Statement-clearParametersToNull.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as integer), cast(? as integer) from rdb$database"};
	select.setInt32(0, 1);
	select.setInt32(1, 2);
	select.clearParameters();
	select.execute(transaction);
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK(select.isNull(0));
	BOOST_CHECK(select.isNull(1));
}

BOOST_AUTO_TEST_CASE(isNullDetectsNullColumn)
{
	const auto database = getTempFile("Statement-isNullDetectsNullColumn.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(null as integer) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK_EQUAL(select.isNull(0), true);
}

BOOST_AUTO_TEST_CASE(nullRoundTrip)
{
	const auto database = getTempFile("Statement-nullRoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction,
		"select cast(? as integer), cast(? as varchar(50)), cast(? as double precision), cast(? as date) from "
		"rdb$database"};
	select.setInt32(0, std::nullopt);
	select.setString(1, std::nullopt);
	select.setDouble(2, std::nullopt);
	select.setDate(3, std::nullopt);
	select.execute(transaction);
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK(!select.getInt32(0).has_value());
	BOOST_CHECK(!select.getString(1).has_value());
	BOOST_CHECK(!select.getDouble(2).has_value());
	BOOST_CHECK(!select.getDate(3).has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementBoolSuite)

BOOST_AUTO_TEST_CASE(setBoolToBoolean)
{
	const auto database = getTempFile("Statement-setBoolToBoolean.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as boolean) from rdb$database"};
	stmt.setBool(0, true);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getBool(0).value(), true);
}

BOOST_AUTO_TEST_CASE(getBoolFromBoolean)
{
	const auto database = getTempFile("Statement-getBoolFromBoolean.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as boolean) from rdb$database"};
	stmt.setBool(0, false);
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getBool(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), false);
}

BOOST_AUTO_TEST_CASE(setStringToBoolColumn)
{
	const auto database = getTempFile("Statement-setStringToBoolColumn.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as boolean) from rdb$database"};
	stmt.setString(0, "true");
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getBool(0).value(), true);
}

BOOST_AUTO_TEST_CASE(getStringFromBoolColumn)
{
	const auto database = getTempFile("Statement-getStringFromBoolColumn.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as boolean) from rdb$database"};
	stmt.setBool(0, true);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "true");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementIntegerSuite)

BOOST_AUTO_TEST_CASE(setInt16ToSmallint)
{
	const auto database = getTempFile("Statement-setInt16ToSmallint.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as smallint) from rdb$database"};
	stmt.setInt16(0, static_cast<std::int16_t>(12345));
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt16(0).value(), 12345);
}

BOOST_AUTO_TEST_CASE(setInt32ToInteger)
{
	const auto database = getTempFile("Statement-setInt32ToInteger.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};
	stmt.setInt32(0, 123456789);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 123456789);
}

BOOST_AUTO_TEST_CASE(setInt64ToBigint)
{
	const auto database = getTempFile("Statement-setInt64ToBigint.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as bigint) from rdb$database"};
	stmt.setInt64(0, 9223372036854775807LL);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt64(0).value(), 9223372036854775807LL);
}

BOOST_AUTO_TEST_CASE(setInt16ToInteger)
{
	const auto database = getTempFile("Statement-setInt16ToInteger.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};
	stmt.setInt16(0, static_cast<std::int16_t>(1234));
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 1234);
}

BOOST_AUTO_TEST_CASE(setInt32ToBigint)
{
	const auto database = getTempFile("Statement-setInt32ToBigint.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as bigint) from rdb$database"};
	stmt.setInt32(0, 123456789);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt64(0).value(), 123456789LL);
}

BOOST_AUTO_TEST_CASE(getInt32FromSmallint)
{
	const auto database = getTempFile("Statement-getInt32FromSmallint.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as smallint) from rdb$database"};
	stmt.setInt16(0, static_cast<std::int16_t>(999));
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 999);
}

BOOST_AUTO_TEST_CASE(getInt64FromInteger)
{
	const auto database = getTempFile("Statement-getInt64FromInteger.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};
	stmt.setInt32(0, 123456);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt64(0).value(), 123456LL);
}

BOOST_AUTO_TEST_CASE(setStringToIntColumn)
{
	const auto database = getTempFile("Statement-setStringToIntColumn.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};
	stmt.setString(0, "123456");
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 123456);
}

BOOST_AUTO_TEST_CASE(getStringFromIntColumn)
{
	const auto database = getTempFile("Statement-getStringFromIntColumn.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};
	stmt.setInt32(0, 987654);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "987654");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementScaledIntegerSuite)

BOOST_AUTO_TEST_CASE(setScaledInt64ToNumeric)
{
	const auto database = getTempFile("Statement-setScaledInt64ToNumeric.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as numeric(18,2)) from rdb$database"};
	stmt.setScaledInt64(0, ScaledInt64{12345, -2});  // 123.45
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getScaledInt64(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->value, 12345);
	BOOST_CHECK_EQUAL(result->scale, -2);
}

BOOST_AUTO_TEST_CASE(getScaledInt64FromNumeric)
{
	const auto database = getTempFile("Statement-getScaledInt64FromNumeric.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(123.4567 as numeric(18,4)) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getScaledInt64(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->value, 1234567);
	BOOST_CHECK_EQUAL(result->scale, -4);
}

BOOST_AUTO_TEST_CASE(setInt32ToNumeric)
{
	const auto database = getTempFile("Statement-setInt32ToNumeric.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as numeric(10,2)) from rdb$database"};
	stmt.setInt32(0, 100);  // Auto-scales to 100.00
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getScaledInt64(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->value, 10000);  // 100.00 as scaled
	BOOST_CHECK_EQUAL(result->scale, -2);
}

BOOST_AUTO_TEST_CASE(setStringToNumeric)
{
	const auto database = getTempFile("Statement-setStringToNumeric.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as numeric(18,2)) from rdb$database"};
	stmt.setString(0, "123.45");
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getScaledInt64(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->value, 12345);
	BOOST_CHECK_EQUAL(result->scale, -2);
}

BOOST_AUTO_TEST_CASE(getStringFromNumeric)
{
	const auto database = getTempFile("Statement-getStringFromNumeric.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(456.78 as numeric(18,2)) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "456.78");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementFloatSuite)

BOOST_AUTO_TEST_CASE(setFloatToFloat)
{
	const auto database = getTempFile("Statement-setFloatToFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, 3.14f);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_CLOSE(stmt.getFloat(0).value(), 3.14f, 0.01f);
}

BOOST_AUTO_TEST_CASE(setDoubleToDoublePrecision)
{
	const auto database = getTempFile("Statement-setDoubleToDoublePrecision.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setDouble(0, 3.141592653589793);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_CLOSE(stmt.getDouble(0).value(), 3.141592653589793, 0.0000001);
}

BOOST_AUTO_TEST_CASE(setFloatToDouble)
{
	const auto database = getTempFile("Statement-setFloatToDouble.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setFloat(0, 2.5f);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_CLOSE(stmt.getDouble(0).value(), 2.5, 0.0001);
}

BOOST_AUTO_TEST_CASE(setInt32ToFloat)
{
	const auto database = getTempFile("Statement-setInt32ToFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setInt32(0, 42);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_CLOSE(stmt.getFloat(0).value(), 42.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(setStringToFloat)
{
	const auto database = getTempFile("Statement-setStringToFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setString(0, "3.14");
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_CLOSE(stmt.getDouble(0).value(), 3.14, 0.001);
}

BOOST_AUTO_TEST_CASE(getStringFromFloat)
{
	const auto database = getTempFile("Statement-getStringFromFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(2.5 as double precision) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getString(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(!result->empty());
}

BOOST_AUTO_TEST_CASE(getDoubleFromFloat)
{
	const auto database = getTempFile("Statement-getDoubleFromFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, 1.5f);
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_CLOSE(stmt.getDouble(0).value(), 1.5, 0.001);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementInfNaNSuite)

BOOST_AUTO_TEST_CASE(setFloatInfToFloat)
{
	const auto database = getTempFile("Statement-setFloatInfToFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, std::numeric_limits<float>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getFloat(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(std::isinf(result.value()));
	BOOST_CHECK(result.value() > 0);
}

BOOST_AUTO_TEST_CASE(setFloatNegInfToFloat)
{
	const auto database = getTempFile("Statement-setFloatNegInfToFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, -std::numeric_limits<float>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getFloat(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(std::isinf(result.value()));
	BOOST_CHECK(result.value() < 0);
}

BOOST_AUTO_TEST_CASE(setFloatNaNToFloat)
{
	const auto database = getTempFile("Statement-setFloatNaNToFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, std::numeric_limits<float>::quiet_NaN());
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getFloat(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(std::isnan(result.value()));
}

BOOST_AUTO_TEST_CASE(setDoubleInfToDouble)
{
	const auto database = getTempFile("Statement-setDoubleInfToDouble.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setDouble(0, std::numeric_limits<double>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getDouble(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(std::isinf(result.value()));
	BOOST_CHECK(result.value() > 0);
}

BOOST_AUTO_TEST_CASE(setDoubleNegInfToDouble)
{
	const auto database = getTempFile("Statement-setDoubleNegInfToDouble.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setDouble(0, -std::numeric_limits<double>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getDouble(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(std::isinf(result.value()));
	BOOST_CHECK(result.value() < 0);
}

BOOST_AUTO_TEST_CASE(setDoubleNaNToDouble)
{
	const auto database = getTempFile("Statement-setDoubleNaNToDouble.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setDouble(0, std::numeric_limits<double>::quiet_NaN());
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getDouble(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(std::isnan(result.value()));
}

BOOST_AUTO_TEST_CASE(getStringFromFloatInf)
{
	const auto database = getTempFile("Statement-getStringFromFloatInf.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, std::numeric_limits<float>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "Infinity");
}

BOOST_AUTO_TEST_CASE(getStringFromFloatNegInf)
{
	const auto database = getTempFile("Statement-getStringFromFloatNegInf.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, -std::numeric_limits<float>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "-Infinity");
}

BOOST_AUTO_TEST_CASE(getStringFromFloatNaN)
{
	const auto database = getTempFile("Statement-getStringFromFloatNaN.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as float) from rdb$database"};
	stmt.setFloat(0, std::numeric_limits<float>::quiet_NaN());
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "NaN");
}

BOOST_AUTO_TEST_CASE(getStringFromDoubleInf)
{
	const auto database = getTempFile("Statement-getStringFromDoubleInf.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setDouble(0, std::numeric_limits<double>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "Infinity");
}

BOOST_AUTO_TEST_CASE(getStringFromDoubleNegInf)
{
	const auto database = getTempFile("Statement-getStringFromDoubleNegInf.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setDouble(0, -std::numeric_limits<double>::infinity());
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "-Infinity");
}

BOOST_AUTO_TEST_CASE(getStringFromDoubleNaN)
{
	const auto database = getTempFile("Statement-getStringFromDoubleNaN.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as double precision) from rdb$database"};
	stmt.setDouble(0, std::numeric_limits<double>::quiet_NaN());
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "NaN");
}

BOOST_AUTO_TEST_CASE(setInfToIntegerThrows)
{
	const auto database = getTempFile("Statement-setInfToIntegerThrows.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};
	BOOST_CHECK_THROW(stmt.setDouble(0, std::numeric_limits<double>::infinity()), DatabaseException);
}

BOOST_AUTO_TEST_CASE(setNaNToIntegerThrows)
{
	const auto database = getTempFile("Statement-setNaNToIntegerThrows.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};
	BOOST_CHECK_THROW(stmt.setDouble(0, std::numeric_limits<double>::quiet_NaN()), DatabaseException);
}

BOOST_AUTO_TEST_CASE(setInfToNumericThrows)
{
	const auto database = getTempFile("Statement-setInfToNumericThrows.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as numeric(18,2)) from rdb$database"};
	BOOST_CHECK_THROW(stmt.setDouble(0, std::numeric_limits<double>::infinity()), DatabaseException);
}

BOOST_AUTO_TEST_CASE(setNaNToNumericThrows)
{
	const auto database = getTempFile("Statement-setNaNToNumericThrows.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as numeric(18,2)) from rdb$database"};
	BOOST_CHECK_THROW(stmt.setDouble(0, std::numeric_limits<double>::quiet_NaN()), DatabaseException);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementStringSuite)

BOOST_AUTO_TEST_CASE(setStringToVarchar)
{
	const auto database = getTempFile("Statement-setStringToVarchar.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as varchar(100)) from rdb$database"};
	stmt.setString(0, "Hello, Firebird!");
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "Hello, Firebird!");
}

BOOST_AUTO_TEST_CASE(getStringFromVarchar)
{
	const auto database = getTempFile("Statement-getStringFromVarchar.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast('Test String' as varchar(50)) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "Test String");
}

BOOST_AUTO_TEST_CASE(stringTruncationThrows)
{
	const auto database = getTempFile("Statement-stringTruncationThrows.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as varchar(3)) from rdb$database"};
	BOOST_CHECK_THROW(stmt.setString(0, "This is too long"), DatabaseException);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementDateTimeSuite)

BOOST_AUTO_TEST_CASE(setDateToDate)
{
	const auto database = getTempFile("Statement-setDateToDate.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	using namespace std::chrono;
	const Date testDate = 2024y / January / 15d;

	Statement stmt{attachment, transaction, "select cast(? as date) from rdb$database"};
	stmt.setDate(0, testDate);
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getDate(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == testDate);
}

BOOST_AUTO_TEST_CASE(getDateFromDate)
{
	const auto database = getTempFile("Statement-getDateFromDate.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast('2024-06-20' as date) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getDate(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result.value() == 2024y / June / 20d);
}

BOOST_AUTO_TEST_CASE(setTimeToTime)
{
	const auto database = getTempFile("Statement-setTimeToTime.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	using namespace std::chrono;
	const Time testTime{hours{14} + minutes{30} + seconds{45}};

	Statement stmt{attachment, transaction, "select cast(? as time) from rdb$database"};
	stmt.setTime(0, testTime);
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTime(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value().to_duration() == testTime.to_duration());
}

BOOST_AUTO_TEST_CASE(setTimestampToTimestamp)
{
	const auto database = getTempFile("Statement-setTimestampToTimestamp.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	using namespace std::chrono;
	const auto testDate = 2024y / March / 15d;
	const Time testTime{hours{10} + minutes{30} + seconds{0}};
	const Timestamp testTimestamp{static_cast<sys_days>(testDate), testTime};

	Statement stmt{attachment, transaction, "select cast(? as timestamp) from rdb$database"};
	stmt.setTimestamp(0, testTimestamp);
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTimestamp(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result->date == static_cast<sys_days>(testDate));
	BOOST_CHECK(result->time.to_duration() == testTime.to_duration());
}

BOOST_AUTO_TEST_CASE(setTimeTzToTimeTz)
{
	const auto database = getTempFile("Statement-setTimeTzToTimeTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	using namespace std::chrono;
	TimeTz testTimeTz;
	testTimeTz.utcTime = Time{hours{12} + minutes{0} + seconds{0}};
	testTimeTz.zone = "UTC";

	Statement stmt{attachment, transaction, "select cast(? as time with time zone) from rdb$database"};
	stmt.setTimeTz(0, testTimeTz);
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTimeTz(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result->utcTime.to_duration() == testTimeTz.utcTime.to_duration());
}

BOOST_AUTO_TEST_CASE(setTimestampTzToTimestampTz)
{
	const auto database = getTempFile("Statement-setTimestampTzToTimestampTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	using namespace std::chrono;
	TimestampTz testTimestampTz;
	testTimestampTz.utcTimestamp.date = 2024y / July / 4d;
	testTimestampTz.utcTimestamp.time = Time{hours{15} + minutes{30} + seconds{0}};
	testTimestampTz.zone = "UTC";

	Statement stmt{attachment, transaction, "select cast(? as timestamp with time zone) from rdb$database"};
	stmt.setTimestampTz(0, testTimestampTz);
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTimestampTz(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result->utcTimestamp.date == testTimestampTz.utcTimestamp.date);
	BOOST_CHECK(result->utcTimestamp.time.to_duration() == testTimestampTz.utcTimestamp.time.to_duration());
}

BOOST_AUTO_TEST_CASE(setStringToDate)
{
	const auto database = getTempFile("Statement-setStringToDate.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as date) from rdb$database"};
	stmt.setString(0, "2024-01-15");
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getDate(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result.value() == 2024y / January / 15d);
}

BOOST_AUTO_TEST_CASE(getStringFromDate)
{
	const auto database = getTempFile("Statement-getStringFromDate.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast('2024-12-25' as date) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "2024-12-25");
}

BOOST_AUTO_TEST_CASE(setStringToTime)
{
	const auto database = getTempFile("Statement-setStringToTime.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as time) from rdb$database"};
	stmt.setString(0, "14:30:45.1234");
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTime(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result.value().to_duration() == (14h + 30min + 45s + 123400us));
}

BOOST_AUTO_TEST_CASE(getStringFromTime)
{
	const auto database = getTempFile("Statement-getStringFromTime.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast('10:20:30.5000' as time) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "10:20:30.5000");
}

BOOST_AUTO_TEST_CASE(setStringToTimestamp)
{
	const auto database = getTempFile("Statement-setStringToTimestamp.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as timestamp) from rdb$database"};
	stmt.setString(0, "2024-05-10 08:30:00");
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTimestamp(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result->date == static_cast<sys_days>(2024y / May / 10d));
}

BOOST_AUTO_TEST_CASE(getStringFromTimestamp)
{
	const auto database = getTempFile("Statement-getStringFromTimestamp.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast('2024-07-15 16:45:30.1234' as timestamp) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "2024-07-15 16:45:30.1234");
}

BOOST_AUTO_TEST_CASE(setStringToTimeTz)
{
	const auto database = getTempFile("Statement-setStringToTimeTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as time with time zone) from rdb$database"};
	stmt.setString(0, "14:30:45.1234 UTC");
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTimeTz(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->zone, "UTC");
}

BOOST_AUTO_TEST_CASE(getStringFromTimeTz)
{
	const auto database = getTempFile("Statement-getStringFromTimeTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{
		attachment, transaction, "select cast('10:20:30.5000 UTC' as time with time zone) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "10:20:30.5000 UTC");
}

BOOST_AUTO_TEST_CASE(setStringToTimestampTz)
{
	const auto database = getTempFile("Statement-setStringToTimestampTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction, "select cast(? as timestamp with time zone) from rdb$database"};
	stmt.setString(0, "2024-05-10 08:30:00.1234 UTC");
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getTimestampTz(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->zone, "UTC");

	using namespace std::chrono;
	BOOST_CHECK(result->utcTimestamp.date == static_cast<sys_days>(2024y / May / 10d));
}

BOOST_AUTO_TEST_CASE(getStringFromTimestampTz)
{
	const auto database = getTempFile("Statement-getStringFromTimestampTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement stmt{attachment, transaction,
		"select cast('2024-07-15 16:45:30.1234 UTC' as timestamp with time zone) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_CHECK_EQUAL(stmt.getString(0).value(), "2024-07-15 16:45:30.1234 UTC");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementBlobSuite)

BOOST_AUTO_TEST_CASE(setBlobIdToBlob)
{
	const auto database = getTempFile("Statement-setBlobIdToBlob.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const std::string testData = "Test blob data";

	Blob writer{attachment, transaction};
	const std::span<const char> dataSpan{testData.data(), testData.size()};
	writer.write(std::as_bytes(dataSpan));
	writer.close();
	BlobId blobId = writer.getId();

	Statement select{attachment, transaction, "select cast(? as blob) from rdb$database"};
	select.setBlobId(0, blobId);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBlobId(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(!result->isEmpty());
}

BOOST_AUTO_TEST_CASE(getBlobIdFromBlob)
{
	const auto database = getTempFile("Statement-getBlobIdFromBlob.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const std::string testData = "Reading blob test";

	Blob writer{attachment, transaction};
	const std::span<const char> dataSpan{testData.data(), testData.size()};
	writer.write(std::as_bytes(dataSpan));
	writer.close();

	Statement select{attachment, transaction, "select cast(? as blob) from rdb$database"};
	select.setBlobId(0, writer.getId());
	BOOST_REQUIRE(select.execute(transaction));
	auto blobId = select.getBlobId(0);
	BOOST_REQUIRE(blobId.has_value());

	Blob reader{attachment, transaction, blobId.value()};
	std::vector<std::byte> buffer(testData.size());
	const auto bytesRead = reader.read(buffer);
	std::string readData(reinterpret_cast<const char*>(buffer.data()), bytesRead);
	BOOST_CHECK_EQUAL(readData, testData);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementCursorSuite)

BOOST_AUTO_TEST_CASE(fetchNextIteratesRows)
{
	const auto database = getTempFile("Statement-fetchNextIteratesRows.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddl{attachment, transaction, "create table t (col integer)"};
	ddl.execute(transaction);
	transaction.commitRetaining();

	Statement insert{attachment, transaction, "insert into t (col) values (?)"};
	for (int i = 1; i <= 5; ++i)
	{
		insert.setInt32(0, i);
		insert.execute(transaction);
	}

	Statement select{attachment, transaction, "select col from t order by col"};
	BOOST_REQUIRE(select.execute(transaction));

	int count = 0;
	int expected = 1;
	do
	{
		++count;
		BOOST_CHECK_EQUAL(select.getInt32(0).value(), expected);
		++expected;
	} while (select.fetchNext());

	BOOST_CHECK_EQUAL(count, 5);
}

BOOST_AUTO_TEST_CASE(fetchReturnsFalseAtEnd)
{
	const auto database = getTempFile("Statement-fetchReturnsFalseAtEnd.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select 1 from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK_EQUAL(select.fetchNext(), false);
}

BOOST_AUTO_TEST_CASE(cursorMethodsReturnFalseWithoutResultSet)
{
	const auto database = getTempFile("Statement-cursorMethodsReturnFalseWithoutResultSet.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddl{attachment, transaction, "create table t (col integer)"};
	ddl.execute(transaction);
	transaction.commitRetaining();

	Statement insert{attachment, transaction, "insert into t (col) values (?)"};
	insert.setInt32(0, 1);
	BOOST_CHECK(insert.execute(transaction));

	BOOST_CHECK_EQUAL(insert.fetchNext(), false);
	BOOST_CHECK_EQUAL(insert.fetchPrior(), false);
	BOOST_CHECK_EQUAL(insert.fetchFirst(), false);
	BOOST_CHECK_EQUAL(insert.fetchLast(), false);
	BOOST_CHECK_EQUAL(insert.fetchAbsolute(1), false);
	BOOST_CHECK_EQUAL(insert.fetchRelative(1), false);
}

BOOST_AUTO_TEST_SUITE_END()


#if FB_CPP_USE_BOOST_MULTIPRECISION != 0

BOOST_AUTO_TEST_SUITE(StatementInt128Suite)

BOOST_AUTO_TEST_CASE(setBoostInt128ToInt128)
{
	const auto database = getTempFile("Statement-setBoostInt128ToInt128.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const BoostInt128 testValue{"170141183460469231731687303715884105727"};

	Statement stmt{attachment, transaction, "select cast(? as int128) from rdb$database"};
	stmt.setBoostInt128(0, testValue);
	BOOST_REQUIRE(stmt.execute(transaction));
	auto result = stmt.getBoostInt128(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == testValue);
}

BOOST_AUTO_TEST_CASE(getBoostInt128FromInt128)
{
	const auto database = getTempFile("Statement-getBoostInt128FromInt128.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(12345678901234567890 as int128) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostInt128(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == BoostInt128{"12345678901234567890"});
}

BOOST_AUTO_TEST_CASE(setScaledBoostInt128ToNumeric38)
{
	const auto database = getTempFile("Statement-setScaledBoostInt128ToNumeric38.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	ScaledBoostInt128 testValue{BoostInt128{"123456789012345678901234"}, -4};

	Statement insert{attachment, transaction, "select cast(? as numeric(38,4)) from rdb$database"};
	insert.setScaledBoostInt128(0, testValue);
	insert.execute(transaction);

	auto result = insert.getScaledBoostInt128(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result->value == testValue.value);
	BOOST_CHECK_EQUAL(result->scale, -4);
}

BOOST_AUTO_TEST_CASE(getScaledBoostInt128FromNumeric38)
{
	const auto database = getTempFile("Statement-getScaledBoostInt128FromNumeric38.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{
		attachment, transaction, "select cast(12345678901234567890.123456 as numeric(38,6)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getScaledBoostInt128(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->scale, -6);
}

BOOST_AUTO_TEST_CASE(setInt64ToInt128)
{
	const auto database = getTempFile("Statement-setInt64ToInt128.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as int128) from rdb$database"};
	select.setInt64(0, 9223372036854775807LL);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostInt128(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == BoostInt128{"9223372036854775807"});
}

BOOST_AUTO_TEST_CASE(getStringFromInt128)
{
	const auto database = getTempFile("Statement-getStringFromInt128.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(12345678901234567890 as int128) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getString(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), "12345678901234567890");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementDecFloatSuite)

BOOST_AUTO_TEST_CASE(setBoostDecFloat16ToDecFloat16)
{
	const auto database = getTempFile("Statement-setBoostDecFloat16ToDecFloat16.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const BoostDecFloat16 testValue{"1234567890123456"};

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setBoostDecFloat16(0, testValue);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == testValue);
}

BOOST_AUTO_TEST_CASE(getBoostDecFloat16FromDecFloat16)
{
	const auto database = getTempFile("Statement-getBoostDecFloat16FromDecFloat16.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(9876543210.12345 as decfloat(16)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(setBoostDecFloat34ToDecFloat34)
{
	const auto database = getTempFile("Statement-setBoostDecFloat34ToDecFloat34.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const BoostDecFloat34 testValue{"1234567890123456789012345678901234"};

	Statement select{attachment, transaction, "select cast(? as decfloat(34)) from rdb$database"};
	select.setBoostDecFloat34(0, testValue);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat34(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == testValue);
}

BOOST_AUTO_TEST_CASE(getBoostDecFloat34FromDecFloat34)
{
	const auto database = getTempFile("Statement-getBoostDecFloat34FromDecFloat34.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction,
		"select cast(9876543210987654321098765432109876.5432 as decfloat(34)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat34(0);
	BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(setDoubleToDecFloat)
{
	const auto database = getTempFile("Statement-setDoubleToDecFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setDouble(0, 123.456);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(static_cast<double>(result.value()), 123.456, 0.001);
}

BOOST_AUTO_TEST_CASE(getStringFromDecFloat)
{
	const auto database = getTempFile("Statement-getStringFromDecFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(123.456 as decfloat(16)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getString(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(!result->empty());
}

BOOST_AUTO_TEST_CASE(setStringToDecFloat16)
{
	const auto database = getTempFile("Statement-setStringToDecFloat16.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setString(0, "9876543210.12345");
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(static_cast<double>(result.value()), 9876543210.12345, 0.00001);
}

BOOST_AUTO_TEST_CASE(setStringToDecFloat34)
{
	const auto database = getTempFile("Statement-setStringToDecFloat34.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(34)) from rdb$database"};
	select.setString(0, "12345678901234567890.123456789");
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat34(0);
	BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(setFloatToDecFloat)
{
	const auto database = getTempFile("Statement-setFloatToDecFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setFloat(0, 3.14f);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(static_cast<double>(result.value()), 3.14, 0.01);
}

BOOST_AUTO_TEST_CASE(setInt32ToDecFloat)
{
	const auto database = getTempFile("Statement-setInt32ToDecFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setInt32(0, 123456789);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == BoostDecFloat16{"123456789"});
}

BOOST_AUTO_TEST_CASE(setInt64ToDecFloat)
{
	const auto database = getTempFile("Statement-setInt64ToDecFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(34)) from rdb$database"};
	select.setInt64(0, 9223372036854775807LL);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat34(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == BoostDecFloat34{"9223372036854775807"});
}

BOOST_AUTO_TEST_CASE(getDoubleFromDecFloat)
{
	const auto database = getTempFile("Statement-getDoubleFromDecFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(3.141592653589793 as decfloat(16)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getDouble(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(result.value(), 3.141592653589793, 0.0000001);
}

BOOST_AUTO_TEST_CASE(getFloatFromDecFloat)
{
	const auto database = getTempFile("Statement-getFloatFromDecFloat.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(2.5 as decfloat(16)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getFloat(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(result.value(), 2.5f, 0.01f);
}

BOOST_AUTO_TEST_CASE(setBoostDecFloat16ToDecFloat34)
{
	const auto database = getTempFile("Statement-setBoostDecFloat16ToDecFloat34.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const BoostDecFloat16 testValue{"1234567890123456"};

	Statement select{attachment, transaction, "select cast(? as decfloat(34)) from rdb$database"};
	select.setBoostDecFloat16(0, testValue);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat34(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == BoostDecFloat34{"1234567890123456"});
}

BOOST_AUTO_TEST_CASE(setBoostDecFloat34ToDecFloat16)
{
	const auto database = getTempFile("Statement-setBoostDecFloat34ToDecFloat16.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	const BoostDecFloat34 testValue{"123456.789"};

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setBoostDecFloat34(0, testValue);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getBoostDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == BoostDecFloat16{"123456.789"});
}

BOOST_AUTO_TEST_SUITE_END()

#endif  // FB_CPP_USE_BOOST_MULTIPRECISION


BOOST_AUTO_TEST_SUITE(StatementOpaqueDateSuite)

BOOST_AUTO_TEST_CASE(setOpaqueDateToDate)
{
	const auto database = getTempFile("Statement-setOpaqueDateToDate.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	using namespace std::chrono;
	const Date testDate = 2024y / January / 15d;

	Statement selectKnown{attachment, transaction, "select date '2024-01-15' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueDate = selectKnown.getOpaqueDate(0);
	BOOST_REQUIRE(expectedOpaqueDate.has_value());

	Statement select{attachment, transaction, "select cast(? as date) from rdb$database"};
	select.setOpaqueDate(0, expectedOpaqueDate.value());
	BOOST_REQUIRE(select.execute(transaction));
	const auto result = select.getDate(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == testDate);
}

BOOST_AUTO_TEST_CASE(getOpaqueDateFromDate)
{
	const auto database = getTempFile("Statement-getOpaqueDateFromDate.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select date '2024-06-20' from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDate(0);
	BOOST_REQUIRE(result.has_value());

	Statement selectKnown{attachment, transaction, "select date '2024-06-20' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	auto expected = selectKnown.getOpaqueDate(0);
	BOOST_REQUIRE(expected.has_value());
	BOOST_CHECK(result.value() == expected.value());
}

BOOST_AUTO_TEST_CASE(opaqueDateRoundTrip)
{
	const auto database = getTempFile("Statement-opaqueDateRoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select date '2024-12-31' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getOpaqueDate(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as date) from rdb$database"};
	select.setOpaqueDate(0, originalOpaque.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDate(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == originalOpaque.value());
}

BOOST_AUTO_TEST_CASE(opaqueDateNullHandling)
{
	const auto database = getTempFile("Statement-opaqueDateNullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as date) from rdb$database"};
	select.setOpaqueDate(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDate(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementOpaqueTimeSuite)

BOOST_AUTO_TEST_CASE(setOpaqueTimeToTime)
{
	const auto database = getTempFile("Statement-setOpaqueTimeToTime.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select time '14:30:45.1234' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueTime = selectKnown.getOpaqueTime(0);
	BOOST_REQUIRE(expectedOpaqueTime.has_value());

	Statement select{attachment, transaction, "select cast(? as time) from rdb$database"};
	select.setOpaqueTime(0, expectedOpaqueTime.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getTime(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result.value().to_duration() == (14h + 30min + 45s + 123400us));
}

BOOST_AUTO_TEST_CASE(getOpaqueTimeFromTime)
{
	const auto database = getTempFile("Statement-getOpaqueTimeFromTime.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select time '10:20:30.5000' from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTime(0);
	BOOST_REQUIRE(result.has_value());

	Statement selectKnown{attachment, transaction, "select time '10:20:30.5000' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	auto expected = selectKnown.getOpaqueTime(0);
	BOOST_REQUIRE(expected.has_value());
	BOOST_CHECK(result.value() == expected.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimeRoundTrip)
{
	const auto database = getTempFile("Statement-opaqueTimeRoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select time '23:59:59.9999' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getOpaqueTime(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as time) from rdb$database"};
	select.setOpaqueTime(0, originalOpaque.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTime(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == originalOpaque.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimeNullHandling)
{
	const auto database = getTempFile("Statement-opaqueTimeNullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as time) from rdb$database"};
	select.setOpaqueTime(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTime(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementOpaqueTimestampSuite)

BOOST_AUTO_TEST_CASE(setOpaqueTimestampToTimestamp)
{
	const auto database = getTempFile("Statement-setOpaqueTimestampToTimestamp.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select timestamp '2024-03-15 10:30:00.5000' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueTimestamp = selectKnown.getOpaqueTimestamp(0);
	BOOST_REQUIRE(expectedOpaqueTimestamp.has_value());

	Statement select{attachment, transaction, "select cast(? as timestamp) from rdb$database"};
	select.setOpaqueTimestamp(0, expectedOpaqueTimestamp.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getTimestamp(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result->date == static_cast<sys_days>(2024y / March / 15d));
	BOOST_CHECK(result->time.to_duration() == (10h + 30min + 0s + 500000us));
}

BOOST_AUTO_TEST_CASE(getOpaqueTimestampFromTimestamp)
{
	const auto database = getTempFile("Statement-getOpaqueTimestampFromTimestamp.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select timestamp '2024-07-15 16:45:30.1234' from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimestamp(0);
	BOOST_REQUIRE(result.has_value());

	Statement selectKnown{attachment, transaction, "select timestamp '2024-07-15 16:45:30.1234' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	auto expected = selectKnown.getOpaqueTimestamp(0);
	BOOST_REQUIRE(expected.has_value());
	BOOST_CHECK(result.value() == expected.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimestampRoundTrip)
{
	const auto database = getTempFile("Statement-opaqueTimestampRoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select timestamp '2024-12-31 23:59:59.9999' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getOpaqueTimestamp(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as timestamp) from rdb$database"};
	select.setOpaqueTimestamp(0, originalOpaque.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimestamp(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == originalOpaque.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimestampNullHandling)
{
	const auto database = getTempFile("Statement-opaqueTimestampNullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as timestamp) from rdb$database"};
	select.setOpaqueTimestamp(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimestamp(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementOpaqueTimeTzSuite)

BOOST_AUTO_TEST_CASE(setOpaqueTimeTzToTimeTz)
{
	const auto database = getTempFile("Statement-setOpaqueTimeTzToTimeTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select time '12:00:00.0000 UTC' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueTimeTz = selectKnown.getOpaqueTimeTz(0);
	BOOST_REQUIRE(expectedOpaqueTimeTz.has_value());

	Statement select{attachment, transaction, "select cast(? as time with time zone) from rdb$database"};
	select.setOpaqueTimeTz(0, expectedOpaqueTimeTz.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getTimeTz(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result->utcTime.to_duration() == (12h + 0min + 0s));
}

BOOST_AUTO_TEST_CASE(getOpaqueTimeTzFromTimeTz)
{
	const auto database = getTempFile("Statement-getOpaqueTimeTzFromTimeTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{
		attachment, transaction, "select cast(time '10:20:30.5000 UTC' as time with time zone) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimeTz(0);
	BOOST_REQUIRE(result.has_value());

	Statement selectKnown{attachment, transaction, "select time '10:20:30.5000 UTC' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	auto expected = selectKnown.getOpaqueTimeTz(0);
	BOOST_REQUIRE(expected.has_value());
	BOOST_CHECK(result.value() == expected.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimeTzRoundTrip)
{
	const auto database = getTempFile("Statement-opaqueTimeTzRoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select time '23:59:59.9999 UTC' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getOpaqueTimeTz(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as time with time zone) from rdb$database"};
	select.setOpaqueTimeTz(0, originalOpaque.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimeTz(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == originalOpaque.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimeTzNullHandling)
{
	const auto database = getTempFile("Statement-opaqueTimeTzNullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as time with time zone) from rdb$database"};
	select.setOpaqueTimeTz(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimeTz(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementOpaqueTimestampTzSuite)

BOOST_AUTO_TEST_CASE(setOpaqueTimestampTzToTimestampTz)
{
	const auto database = getTempFile("Statement-setOpaqueTimestampTzToTimestampTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select timestamp '2024-07-04 15:30:00.0000 UTC' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueTimestampTz = selectKnown.getOpaqueTimestampTz(0);
	BOOST_REQUIRE(expectedOpaqueTimestampTz.has_value());

	Statement select{attachment, transaction, "select cast(? as timestamp with time zone) from rdb$database"};
	select.setOpaqueTimestampTz(0, expectedOpaqueTimestampTz.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getTimestampTz(0);
	BOOST_REQUIRE(result.has_value());

	using namespace std::chrono;
	BOOST_CHECK(result->utcTimestamp.date == 2024y / July / 4d);
	BOOST_CHECK(result->utcTimestamp.time.to_duration() == (15h + 30min + 0s));
}

BOOST_AUTO_TEST_CASE(getOpaqueTimestampTzFromTimestampTz)
{
	const auto database = getTempFile("Statement-getOpaqueTimestampTzFromTimestampTz.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select timestamp '2024-07-15 16:45:30.1234 UTC' from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimestampTz(0);
	BOOST_REQUIRE(result.has_value());

	Statement selectKnown{attachment, transaction, "select timestamp '2024-07-15 16:45:30.1234 UTC' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	auto expected = selectKnown.getOpaqueTimestampTz(0);
	BOOST_REQUIRE(expected.has_value());
	BOOST_CHECK(result.value() == expected.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimestampTzRoundTrip)
{
	const auto database = getTempFile("Statement-opaqueTimestampTzRoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select timestamp '2024-12-31 23:59:59.9999 UTC' from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getOpaqueTimestampTz(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as timestamp with time zone) from rdb$database"};
	select.setOpaqueTimestampTz(0, originalOpaque.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimestampTz(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value() == originalOpaque.value());
}

BOOST_AUTO_TEST_CASE(opaqueTimestampTzNullHandling)
{
	const auto database = getTempFile("Statement-opaqueTimestampTzNullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as timestamp with time zone) from rdb$database"};
	select.setOpaqueTimestampTz(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueTimestampTz(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementOpaqueInt128Suite)

BOOST_AUTO_TEST_CASE(setOpaqueInt128ToInt128)
{
	const auto database = getTempFile("Statement-setOpaqueInt128ToInt128.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select cast(12345678901234567890 as int128) from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueInt128 = selectKnown.getScaledOpaqueInt128(0);
	BOOST_REQUIRE(expectedOpaqueInt128.has_value());

	Statement select{attachment, transaction, "select cast(? as int128) from rdb$database"};
	select.setOpaqueInt128(0, expectedOpaqueInt128->value);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getScaledOpaqueInt128(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(select.getString(0).value(), "12345678901234567890");
}

BOOST_AUTO_TEST_CASE(getScaledOpaqueInt128FromInt128)
{
	const auto database = getTempFile("Statement-getScaledOpaqueInt128FromInt128.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(98765432109876543210 as int128) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getScaledOpaqueInt128(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->scale, 0);
}

BOOST_AUTO_TEST_CASE(opaqueInt128RoundTrip)
{
	const auto database = getTempFile("Statement-opaqueInt128RoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{
		attachment, transaction, "select cast(170141183460469231731687303715884105727 as int128) from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getScaledOpaqueInt128(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as int128) from rdb$database"};
	select.setOpaqueInt128(0, originalOpaque->value);
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK_EQUAL(select.getString(0).value(), "170141183460469231731687303715884105727");
}

BOOST_AUTO_TEST_CASE(opaqueInt128NullHandling)
{
	const auto database = getTempFile("Statement-opaqueInt128NullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as int128) from rdb$database"};
	select.setOpaqueInt128(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getScaledOpaqueInt128(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementOpaqueDecFloat16Suite)

BOOST_AUTO_TEST_CASE(setOpaqueDecFloat16ToDecFloat16)
{
	const auto database = getTempFile("Statement-setOpaqueDecFloat16ToDecFloat16.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select cast(1234567890.12345 as decfloat(16)) from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueDecFloat16 = selectKnown.getOpaqueDecFloat16(0);
	BOOST_REQUIRE(expectedOpaqueDecFloat16.has_value());

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setOpaqueDecFloat16(0, expectedOpaqueDecFloat16.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getString(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(!result->empty());
}

BOOST_AUTO_TEST_CASE(getOpaqueDecFloat16FromDecFloat16)
{
	const auto database = getTempFile("Statement-getOpaqueDecFloat16FromDecFloat16.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(9876543210.12345 as decfloat(16)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(opaqueDecFloat16RoundTrip)
{
	const auto database = getTempFile("Statement-opaqueDecFloat16RoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{attachment, transaction, "select cast(1234567890123456 as decfloat(16)) from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getOpaqueDecFloat16(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setOpaqueDecFloat16(0, originalOpaque.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDecFloat16(0);
	BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(opaqueDecFloat16NullHandling)
{
	const auto database = getTempFile("Statement-opaqueDecFloat16NullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(16)) from rdb$database"};
	select.setOpaqueDecFloat16(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDecFloat16(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StatementOpaqueDecFloat34Suite)

BOOST_AUTO_TEST_CASE(setOpaqueDecFloat34ToDecFloat34)
{
	const auto database = getTempFile("Statement-setOpaqueDecFloat34ToDecFloat34.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{
		attachment, transaction, "select cast(12345678901234567890.123456789 as decfloat(34)) from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto expectedOpaqueDecFloat34 = selectKnown.getOpaqueDecFloat34(0);
	BOOST_REQUIRE(expectedOpaqueDecFloat34.has_value());

	Statement select{attachment, transaction, "select cast(? as decfloat(34)) from rdb$database"};
	select.setOpaqueDecFloat34(0, expectedOpaqueDecFloat34.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getString(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(!result->empty());
}

BOOST_AUTO_TEST_CASE(getOpaqueDecFloat34FromDecFloat34)
{
	const auto database = getTempFile("Statement-getOpaqueDecFloat34FromDecFloat34.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction,
		"select cast(9876543210987654321098765432109876.5432 as decfloat(34)) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDecFloat34(0);
	BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(opaqueDecFloat34RoundTrip)
{
	const auto database = getTempFile("Statement-opaqueDecFloat34RoundTrip.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement selectKnown{
		attachment, transaction, "select cast(1234567890123456789012345678901234 as decfloat(34)) from rdb$database"};
	BOOST_REQUIRE(selectKnown.execute(transaction));
	const auto originalOpaque = selectKnown.getOpaqueDecFloat34(0);
	BOOST_REQUIRE(originalOpaque.has_value());

	Statement select{attachment, transaction, "select cast(? as decfloat(34)) from rdb$database"};
	select.setOpaqueDecFloat34(0, originalOpaque.value());
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDecFloat34(0);
	BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(opaqueDecFloat34NullHandling)
{
	const auto database = getTempFile("Statement-opaqueDecFloat34NullHandling.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement select{attachment, transaction, "select cast(? as decfloat(34)) from rdb$database"};
	select.setOpaqueDecFloat34(0, std::nullopt);
	BOOST_REQUIRE(select.execute(transaction));
	auto result = select.getOpaqueDecFloat34(0);
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StructBindingSuite)

BOOST_AUTO_TEST_CASE(getStructRetrievesAllColumns)
{
	struct Result
	{
		std::optional<std::int32_t> col1;
		std::optional<std::string> col2;
		std::optional<double> col3;
	};

	const auto database = getTempFile("Statement-getStructRetrievesAllColumns.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 42, 'hello', 3.14e0 from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	const auto result = stmt.get<Result>();
	BOOST_CHECK(result.col1.has_value());
	BOOST_CHECK_EQUAL(result.col1.value(), 42);
	BOOST_CHECK(result.col2.has_value());
	BOOST_CHECK_EQUAL(result.col2.value(), "hello");
	BOOST_CHECK(result.col3.has_value());
	BOOST_CHECK_CLOSE(result.col3.value(), 3.14, 0.001);
}

BOOST_AUTO_TEST_CASE(setStructSetsAllParameters)
{
	struct Params
	{
		std::int32_t val1;
		std::string_view val2;
	};

	const auto database = getTempFile("Statement-setStructSetsAllParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(? as integer), cast(? as varchar(50)) from rdb$database"};

	Params params{123, "test"};
	stmt.set(params);
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 123);
	BOOST_CHECK_EQUAL(stmt.getString(1).value(), "test");
}

BOOST_AUTO_TEST_CASE(getStructFieldCountMismatchThrows)
{
	struct WrongSize
	{
		std::optional<std::int32_t> col1;
		std::optional<std::int32_t> col2;
	};

	const auto database = getTempFile("Statement-getStructFieldCountMismatchThrows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 1, 2, 3 from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_THROW(stmt.get<WrongSize>(), FbCppException);
}

BOOST_AUTO_TEST_CASE(setStructFieldCountMismatchThrows)
{
	struct WrongSize
	{
		std::int32_t val1;
		std::int32_t val2;
		std::int32_t val3;
	};

	const auto database = getTempFile("Statement-setStructFieldCountMismatchThrows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};

	WrongSize params{1, 2, 3};
	BOOST_CHECK_THROW(stmt.set(params), FbCppException);
}

BOOST_AUTO_TEST_CASE(nullForNonOptionalFieldThrows)
{
	struct NonOptional
	{
		std::int32_t value;
	};

	const auto database = getTempFile("Statement-nullForNonOptionalFieldThrows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(null as integer) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_THROW(stmt.get<NonOptional>(), FbCppException);
}

BOOST_AUTO_TEST_CASE(mixedOptionalAndNonOptionalFields)
{
	struct Mixed
	{
		std::int32_t required;
		std::optional<std::string> optional;
	};

	const auto database = getTempFile("Statement-mixedOptionalAndNonOptionalFields.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 42, cast(null as varchar(10)) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	const auto result = stmt.get<Mixed>();
	BOOST_CHECK_EQUAL(result.required, 42);
	BOOST_CHECK(!result.optional.has_value());
}

BOOST_AUTO_TEST_CASE(structWithDateTimeFields)
{
	struct DateTimeResult
	{
		std::optional<Date> dateCol;
		std::optional<Time> timeCol;
	};

	const auto database = getTempFile("Statement-structWithDateTimeFields.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{
		attachment, transaction, "select cast('2025-01-15' as date), cast('10:30:00' as time) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	const auto result = stmt.get<DateTimeResult>();
	BOOST_CHECK(result.dateCol.has_value());
	BOOST_CHECK(result.timeCol.has_value());

	const auto& date = result.dateCol.value();
	BOOST_CHECK_EQUAL(static_cast<int>(date.year()), 2025);
	BOOST_CHECK_EQUAL(static_cast<unsigned>(date.month()), 1);
	BOOST_CHECK_EQUAL(static_cast<unsigned>(date.day()), 15);
}

BOOST_AUTO_TEST_CASE(setStructWithOptionalNull)
{
	struct ParamsWithNull
	{
		std::int32_t val1;
		std::optional<std::string_view> val2;
	};

	const auto database = getTempFile("Statement-setStructWithOptionalNull.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(? as integer), cast(? as varchar(50)) from rdb$database"};

	ParamsWithNull params{999, std::nullopt};
	stmt.set(params);
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 999);
	BOOST_CHECK(!stmt.getString(1).has_value());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(TupleBindingSuite)

BOOST_AUTO_TEST_CASE(getTupleRetrievesAllColumns)
{
	using ResultTuple = std::tuple<std::optional<std::int32_t>, std::optional<std::string>, std::optional<double>>;

	const auto database = getTempFile("Statement-getTupleRetrievesAllColumns.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 42, 'hello', 3.14e0 from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	const auto result = stmt.get<ResultTuple>();
	BOOST_CHECK(std::get<0>(result).has_value());
	BOOST_CHECK_EQUAL(std::get<0>(result).value(), 42);
	BOOST_CHECK(std::get<1>(result).has_value());
	BOOST_CHECK_EQUAL(std::get<1>(result).value(), "hello");
	BOOST_CHECK(std::get<2>(result).has_value());
	BOOST_CHECK_CLOSE(std::get<2>(result).value(), 3.14, 0.001);
}

BOOST_AUTO_TEST_CASE(setTupleSetsAllParameters)
{
	using ParamTuple = std::tuple<std::int32_t, std::string_view>;

	const auto database = getTempFile("Statement-setTupleSetsAllParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(? as integer), cast(? as varchar(50)) from rdb$database"};

	stmt.set(ParamTuple{123, "test"});
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 123);
	BOOST_CHECK_EQUAL(stmt.getString(1).value(), "test");
}

BOOST_AUTO_TEST_CASE(getTupleElementCountMismatchThrows)
{
	using WrongTuple = std::tuple<std::optional<std::int32_t>, std::optional<std::int32_t>>;

	const auto database = getTempFile("Statement-getTupleElementCountMismatchThrows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 1, 2, 3 from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_THROW(stmt.get<WrongTuple>(), FbCppException);
}

BOOST_AUTO_TEST_CASE(setTupleElementCountMismatchThrows)
{
	using WrongTuple = std::tuple<std::int32_t, std::int32_t, std::int32_t>;

	const auto database = getTempFile("Statement-setTupleElementCountMismatchThrows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(? as integer) from rdb$database"};

	BOOST_CHECK_THROW(stmt.set(WrongTuple{1, 2, 3}), FbCppException);
}

BOOST_AUTO_TEST_CASE(nullForNonOptionalTupleElementThrows)
{
	using NonOptionalTuple = std::tuple<std::int32_t>;

	const auto database = getTempFile("Statement-nullForNonOptionalTupleElementThrows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(null as integer) from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_THROW(stmt.get<NonOptionalTuple>(), FbCppException);
}

BOOST_AUTO_TEST_CASE(pairAsResultType)
{
	using ResultPair = std::pair<std::optional<std::int32_t>, std::optional<std::string>>;

	const auto database = getTempFile("Statement-pairAsResultType.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select 100, 'pair test' from rdb$database"};
	BOOST_REQUIRE(stmt.execute(transaction));

	const auto result = stmt.get<ResultPair>();
	BOOST_CHECK(result.first.has_value());
	BOOST_CHECK_EQUAL(result.first.value(), 100);
	BOOST_CHECK(result.second.has_value());
	BOOST_CHECK_EQUAL(result.second.value(), "pair test");
}

BOOST_AUTO_TEST_CASE(setTupleWithOptionalNull)
{
	using ParamTuple = std::tuple<std::int32_t, std::optional<std::string_view>>;

	const auto database = getTempFile("Statement-setTupleWithOptionalNull.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select cast(? as integer), cast(? as varchar(50)) from rdb$database"};

	stmt.set(ParamTuple{999, std::nullopt});
	BOOST_REQUIRE(stmt.execute(transaction));

	BOOST_CHECK_EQUAL(stmt.getInt32(0).value(), 999);
	BOOST_CHECK(!stmt.getString(1).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
