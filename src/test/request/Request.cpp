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

#include "../TestUtil.h"
#include "firebird/impl/blr.h"
#include "fb-cpp/Exception.h"
#include "fb-cpp/request/Request.h"
#include "fb-cpp/Transaction.h"
#include "fb-cpp/request/RequestMessageFormat.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <tuple>
#include <variant>
#include <vector>

using namespace fbcpp::request;


namespace
{
	void appendByte(std::vector<std::uint8_t>& blr, std::uint8_t value)
	{
		blr.push_back(value);
	}

	void appendWord(std::vector<std::uint8_t>& blr, std::uint16_t value)
	{
		blr.push_back(static_cast<std::uint8_t>(value & 0xffu));
		blr.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffu));
	}

	void appendString(std::vector<std::uint8_t>& blr, const std::string_view value)
	{
		blr.push_back(static_cast<std::uint8_t>(value.size()));
		blr.insert(blr.end(), value.begin(), value.end());
	}

	void appendClause(std::vector<std::uint8_t>& blr, const std::vector<std::byte>& clause)
	{
		for (const auto b : clause)
			blr.push_back(static_cast<std::uint8_t>(b));
	}

	std::vector<std::byte> toBytes(const std::vector<std::uint8_t>& blr)
	{
		return std::vector<std::byte>{reinterpret_cast<const std::byte*>(blr.data()),
			reinterpret_cast<const std::byte*>(blr.data() + blr.size())};
	}

	Request makeSetterRequest(Attachment& attachment, const RequestMessageFormat& inputFormat)
	{
		RequestMessageFormat outputFormat;
		outputFormat.addInt32(false);

		std::vector<std::uint8_t> blr{blr_version5, blr_begin};
		appendClause(blr, inputFormat.buildBlrMessageClause(0));
		appendClause(blr, outputFormat.buildBlrMessageClause(1));

		appendByte(blr, blr_send);
		appendByte(blr, 1u);
		appendByte(blr, blr_begin);
		appendByte(blr, 1u);
		appendByte(blr, blr_literal);
		appendByte(blr, blr_long);
		appendByte(blr, 0u);
		for (const auto b : {42u, 0u, 0u, 0u})
			appendByte(blr, static_cast<std::uint8_t>(b));
		appendByte(blr, blr_parameter);
		appendByte(blr, 1u);
		appendWord(blr, 0u);
		appendByte(blr, blr_end);
		appendByte(blr, blr_end);
		appendByte(blr, blr_eoc);

		return Request{attachment, toBytes(blr), {{0u, inputFormat}, {1u, outputFormat}}};
	}

	struct RqtRow final
	{
		std::int32_t id;
		std::optional<std::string> name;
	};

	void createTableAndRows(Attachment& attachment)
	{
		{
			Transaction transaction{attachment};
			attachment.execute(transaction,
				"create table RQT ("
				"  ID integer not null,"
				"  NAME varchar(30),"
				"  CD date,"
				"  constraint pk_rqt primary key (ID))");
			transaction.commit();
		}

		Transaction transaction{attachment};

		for (const auto id : {1, 2, 3, 4, 5})
		{
			Statement statement{attachment, transaction,
				id % 2 == 0 ? "insert into RQT (ID, NAME) values (?, ?)" : "insert into RQT (ID) values (?)"};
			statement.set(0, id);

			if (id % 2 == 0)
				statement.set(1, "name" + std::to_string(id));

			statement.execute(transaction);
		}

		transaction.commit();
	}
}  // namespace


BOOST_AUTO_TEST_SUITE(RequestSuite)

BOOST_AUTO_TEST_CASE(formatComputesLayoutAndGeneratesClause)
{
	RequestMessageFormat format;
	format.addInt32(false);
	format.addString(true, 30);
	format.addDate(false);

	BOOST_REQUIRE_EQUAL(format.getCount(), 3u);
	BOOST_CHECK_EQUAL(format.getLength(), 44u);

	const auto& descriptors = format.getDescriptors();

	BOOST_CHECK(descriptors[0].offset == 0u);
	BOOST_CHECK(descriptors[0].length == 4u);
	BOOST_CHECK(!descriptors[0].isNullable);

	BOOST_CHECK(descriptors[1].offset == 4u);
	BOOST_CHECK(descriptors[1].length == 32u);
	BOOST_CHECK(descriptors[1].nullOffset == 36u);
	BOOST_CHECK(descriptors[1].isNullable);

	BOOST_CHECK(descriptors[2].offset == 40u);
	BOOST_CHECK(descriptors[2].length == 4u);
	BOOST_CHECK(!descriptors[2].isNullable);

	BOOST_CHECK_EQUAL(format.getBlrFieldIndex(0), 0u);
	BOOST_CHECK_EQUAL(format.getBlrFieldIndex(1), 1u);
	BOOST_CHECK_EQUAL(format.getBlrFieldIndex(2), 3u);

	const auto clause = format.buildBlrMessageClause(2);

	static constexpr std::uint8_t expected[] = {
		blr_message,
		2u,  // message number
		4u,
		0u,  // entry count (value + flag + value + value)
		blr_long,
		0u,  // scale 0
		blr_varying,
		30u,
		0u,  // length 30 (dynamic charset)
		blr_short,
		0u,  // null indicator
		blr_sql_date,
	};

	BOOST_REQUIRE_EQUAL(clause.size(), sizeof(expected));
	BOOST_CHECK(std::equal(clause.begin(), clause.end(), expected, expected + sizeof(expected),
		[](std::byte a, std::uint8_t b) { return a == std::byte{b}; }));
}

BOOST_AUTO_TEST_CASE(compileInvalidBlrThrows)
{
	const auto database = getTempFile("Request-compileInvalidBlrThrows.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	BOOST_CHECK_THROW(
		Request(attachment, toBytes(std::vector<std::uint8_t>{blr_version5, 99u, 42u})), DatabaseException);
}

BOOST_AUTO_TEST_CASE(singletonSelectWithLiteral)
{
	const auto database = getTempFile("Request-singletonSelectWithLiteral.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};
	Transaction transaction{attachment};

	RequestMessageFormat outFormat;
	outFormat.addInt32(false);

	std::vector<std::uint8_t> blr{blr_version5, blr_begin};
	appendClause(blr, outFormat.buildBlrMessageClause(1));

	// blr_send 1 { blr_assignment (blr_literal long 42) -> (blr_parameter msg1 #0) }
	appendByte(blr, blr_send);
	appendByte(blr, 1u);
	appendByte(blr, blr_begin);
	appendByte(blr, 1u);
	appendByte(blr, blr_literal);
	appendByte(blr, blr_long);
	appendByte(blr, 0u);
	for (const auto b : {42u, 0u, 0u, 0u})
		appendByte(blr, static_cast<std::uint8_t>(b));
	appendByte(blr, blr_parameter);
	appendByte(blr, 1u);
	appendWord(blr, 0u);
	appendByte(blr, blr_end);
	appendByte(blr, blr_end);
	appendByte(blr, blr_eoc);

	Request request{attachment, toBytes(blr), {{1, outFormat}}};

	request.start(transaction);

	BOOST_CHECK(request.receive(1));
	BOOST_CHECK_EQUAL(request.getMessage(1).getRow().getInt32(0).value(), 42);

	BOOST_CHECK(!request.receive(1));
	BOOST_CHECK_NO_THROW(request.receive(1));
}

BOOST_AUTO_TEST_CASE(insertWithParameters)
{
	const auto database = getTempFile("Request-insertWithParameters.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	createTableAndRows(attachment);

	Transaction transaction{attachment};

	RequestMessageFormat inFormat;
	inFormat.addInt32(false);
	inFormat.addString(true, 30);
	inFormat.addDate(false);

	std::vector<std::uint8_t> blr{blr_version5, blr_begin};
	appendClause(blr, inFormat.buildBlrMessageClause(0));

	// blr_receive 0 { blr_store (blr_relation "RQT" ctx0) { assignments } }
	appendByte(blr, blr_receive);
	appendByte(blr, 0u);
	appendByte(blr, blr_store);
	appendByte(blr, blr_relation);
	appendString(blr, "RQT");
	appendByte(blr, 0u);  // context
	appendByte(blr, blr_begin);

	const auto assignField = [&](unsigned parameterNumber, bool nullable, const std::string_view fieldName)
	{
		appendByte(blr, 1u);  // blr_assignment

		if (nullable)
		{
			appendByte(blr, blr_parameter2);
			appendByte(blr, 0u);
			appendWord(blr, static_cast<std::uint16_t>(parameterNumber));
			appendWord(blr, static_cast<std::uint16_t>(parameterNumber + 1u));
		}
		else
		{
			appendByte(blr, blr_parameter);
			appendByte(blr, 0u);
			appendWord(blr, static_cast<std::uint16_t>(parameterNumber));
		}

		appendByte(blr, blr_field);
		appendByte(blr, 0u);  // context
		appendString(blr, fieldName);
	};

	assignField(0u, false, "ID");
	assignField(1u, true, "NAME");
	assignField(3u, false, "CD");

	appendByte(blr, blr_end);  // assignments
	appendByte(blr, blr_end);  // top begin
	appendByte(blr, blr_eoc);

	Request request{attachment, toBytes(blr), {{0, inFormat}}};

	auto& message = request.getMessage(0);
	message.setInt32(0, 7);
	message.setNull(1);
	message.setDate(
		2, std::chrono::year_month_day{std::chrono::year{2026}, std::chrono::month{8}, std::chrono::day{25}});
	request.startAndSend(transaction, 0);

	message.clearParameters();
	message.setInt32(0, 8);
	message.setString(1, "eight");
	message.setDate(
		2, std::chrono::year_month_day{std::chrono::year{2025}, std::chrono::month{12}, std::chrono::day{31}});
	request.startAndSend(transaction, 0);

	transaction.commit();

	Transaction verification{attachment};

	BOOST_CHECK_EQUAL(
		attachment.queryScalar<std::int32_t>(verification, "select count(*) from RQT where ID in (7, 8)").value(), 2);

	BOOST_CHECK(!attachment.queryScalar<std::string>(verification, "select NAME from RQT where ID = 7").has_value());

	BOOST_CHECK_EQUAL(
		attachment.queryScalar<std::string>(verification, "select NAME from RQT where ID = 8").value(), "eight");

	BOOST_CHECK_EQUAL(
		attachment.queryScalar<std::int32_t>(verification, "select extract(day from CD) from RQT where ID = 7").value(),
		25);

	BOOST_CHECK_EQUAL(
		attachment.queryScalar<std::int32_t>(verification, "select extract(month from CD) from RQT where ID = 8")
			.value(),
		12);
}

BOOST_AUTO_TEST_CASE(multiRowSelectWithFilterAndNullableOutput)
{
	const auto database = getTempFile("Request-multiRowSelect.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	createTableAndRows(attachment);

	Transaction transaction{attachment};

	RequestMessageFormat inFormat;
	inFormat.addInt32(false);

	RequestMessageFormat outFormat;
	outFormat.addInt32(false);
	outFormat.addString(true, 30);

	std::vector<std::uint8_t> blr{blr_version5, blr_begin};
	appendClause(blr, inFormat.buildBlrMessageClause(0));
	appendClause(blr, outFormat.buildBlrMessageClause(1));

	// blr_receive 0
	appendByte(blr, blr_receive);
	appendByte(blr, 0u);
	appendByte(blr, blr_begin);

	//   blr_for (blr_rse count=1 relation RQT ctx0 boolean ID >= param(msg0 #0)) { send msg1 }
	appendByte(blr, blr_for);
	appendByte(blr, blr_rse);
	appendByte(blr, 1u);  // stream count
	appendByte(blr, blr_relation);
	appendString(blr, "RQT");
	appendByte(blr, 0u);  // context
	appendByte(blr, blr_boolean);
	appendByte(blr, blr_geq);
	appendByte(blr, blr_field);
	appendByte(blr, 0u);
	appendString(blr, "ID");
	appendByte(blr, blr_parameter);
	appendByte(blr, 0u);
	appendWord(blr, 0u);
	appendByte(blr, 255u);  // end rse

	appendByte(blr, blr_begin);
	appendByte(blr, blr_send);
	appendByte(blr, 1u);
	appendByte(blr, blr_begin);

	// blr_assignment field ID -> blr_parameter msg1 #0
	appendByte(blr, 1u);
	appendByte(blr, blr_field);
	appendByte(blr, 0u);
	appendString(blr, "ID");
	appendByte(blr, blr_parameter);
	appendByte(blr, 1u);
	appendWord(blr, 0u);

	// blr_assignment field NAME -> blr_parameter2 msg1 #1/#2
	appendByte(blr, 1u);
	appendByte(blr, blr_field);
	appendByte(blr, 0u);
	appendString(blr, "NAME");
	appendByte(blr, blr_parameter2);
	appendByte(blr, 1u);
	appendWord(blr, 1u);
	appendWord(blr, 2u);

	appendByte(blr, blr_end);  // send body
	appendByte(blr, blr_end);  // for body
	appendByte(blr, blr_end);  // receive body
	appendByte(blr, blr_end);  // top begin
	appendByte(blr, blr_eoc);

	Request request{attachment, toBytes(blr), {{0, inFormat}, {1, outFormat}}};

	BOOST_CHECK_EQUAL(request.getMessage(0).getDescriptors().size(), 1u);

	request.getMessage(0).setInt32(0, 3);
	request.startAndSend(transaction, 0);

	std::map<std::int32_t, std::optional<std::string>> rows;

	while (request.receive(1))
	{
		auto& row = request.getMessage(1).getRow();
		rows[row.getInt32(0).value()] = row.getString(1);
	}

	BOOST_REQUIRE_EQUAL(rows.size(), 3u);

	BOOST_REQUIRE(rows.find(3) != rows.end() && !rows[3].has_value());
	BOOST_REQUIRE(rows.find(4) != rows.end() && rows[4].has_value() && rows[4].value() == "name4");
	BOOST_REQUIRE(rows.find(5) != rows.end() && !rows[5].has_value());
}

BOOST_AUTO_TEST_CASE(setterValidation)
{
	const auto database = getTempFile("Request-setterValidation.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};
	Transaction transaction{attachment};

	RequestMessageFormat outFormat;
	outFormat.addInt32(false);

	std::vector<std::uint8_t> blr{blr_version5, blr_begin};
	appendClause(blr, outFormat.buildBlrMessageClause(1));
	appendByte(blr, blr_send);
	appendByte(blr, 1u);
	appendByte(blr, blr_begin);
	appendByte(blr, 1u);
	appendByte(blr, blr_literal);
	appendByte(blr, blr_long);
	appendByte(blr, 0u);
	for (const auto b : {1u, 0u, 0u, 0u})
		appendByte(blr, static_cast<std::uint8_t>(b));
	appendByte(blr, blr_parameter);
	appendByte(blr, 1u);
	appendWord(blr, 0u);
	appendByte(blr, blr_end);
	appendByte(blr, blr_end);
	appendByte(blr, blr_eoc);

	RequestMessageFormat unusedFormat;
	unusedFormat.addInt32(true);

	Request request{attachment, toBytes(blr), {{1, outFormat}, {5, unusedFormat}}};

	auto& message = request.getMessage(5);
	BOOST_CHECK(message.getRow().getInt32(0).has_value() == false);  // starts null
	BOOST_CHECK_NO_THROW(message.setNull(0));
	BOOST_CHECK_THROW(message.setBool(0, true), FbCppException);
	BOOST_CHECK_THROW(request.getMessage(2), FbCppException);
}

BOOST_AUTO_TEST_CASE(sharedSettersHandleRequestMessageLayout)
{
	const auto database = getTempFile("Request-sharedSettersHandleRequestMessageLayout.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	RequestMessageFormat format;
	format.addInt16(false);
	format.addInt32(false);
	format.addScaledInt64(false, -2);
	format.addFloat(false);
	format.addDouble(false);
	format.addDate(false);
	format.addTime(false);
	format.addTimestamp(false);
	format.addTimeTz(false);
	format.addTimestampTz(false);
	format.addDecFloat16(false);
	format.addDecFloat34(false);
	format.addString(true, 6);

	auto request = makeSetterRequest(attachment, format);
	auto& message = request.getMessage(0);

	using namespace std::chrono;
	const auto testDate = 2026y / August / 29d;
	const Time testTime{12h + 34min + 56s + 123400us};
	const Timestamp testTimestamp{static_cast<sys_days>(testDate), testTime};
	TimeTz testTimeTz;
	testTimeTz.utcTime = testTime;
	testTimeTz.zone = "UTC";
	TimestampTz testTimestampTz;
	testTimestampTz.utcTimestamp = testTimestamp;
	testTimestampTz.zone = "UTC";

	message.setString(0, "123");
	message.setInt16(1, 456);
	message.setString(2, "123.45");
	message.setFloat(3, 3.5f);
	message.setDouble(4, 4.5);
	message.setDate(5, testDate);
	message.setTime(6, testTime);
	message.setTimestamp(7, testTimestamp);
	message.setTimeTz(8, testTimeTz);
	message.setTimestampTz(9, testTimestampTz);
	message.setString(10, "987.5");
	message.setString(11, "12345678901234567890.123456789");
	message.setString(12, "abcdef");

	auto& row = message.getRow();
	BOOST_CHECK_EQUAL(row.getInt16(0).value(), 123);
	BOOST_CHECK_EQUAL(row.getInt32(1).value(), 456);
	BOOST_CHECK((row.getScaledInt64(2).value() == ScaledInt64{12345, -2}));
	BOOST_CHECK_CLOSE(row.getFloat(3).value(), 3.5f, 0.001f);
	BOOST_CHECK_CLOSE(row.getDouble(4).value(), 4.5, 0.001);
	BOOST_CHECK(row.getDate(5).value() == testDate);
	BOOST_CHECK(row.getTime(6).value().to_duration() == testTime.to_duration());
	BOOST_CHECK(row.getTimestamp(7).value() == testTimestamp);
	BOOST_CHECK(row.getTimeTz(8).value() == testTimeTz);
	BOOST_CHECK(row.getTimestampTz(9).value() == testTimestampTz);
	BOOST_CHECK(!row.getString(10).value().empty());
	BOOST_CHECK(!row.getString(11).value().empty());
	BOOST_CHECK_EQUAL(row.getString(12).value(), "abcdef");

	const std::vector<std::byte> bytes(6, std::byte{0x01});
	message.setBytes(12, bytes);
	BOOST_CHECK(row.getBytes(12).value() == bytes);
	BOOST_CHECK_THROW(message.setBytes(12, std::vector<std::byte>(7, std::byte{0x01})), DatabaseException);
	BOOST_CHECK_THROW(message.setString(12, "abcdefg"), DatabaseException);

	BOOST_CHECK_EXCEPTION(message.setNull(0), FbCppException,
		[](const FbCppException& e) { return std::string_view{e.what()} == "Field 0 is not nullable"; });
	BOOST_CHECK_EXCEPTION(message.set(0, std::optional<std::int16_t>{}), FbCppException,
		[](const FbCppException& e) { return std::string_view{e.what()} == "Field 0 is not nullable"; });

	message.clearParameters();
	BOOST_CHECK_EQUAL(message.getRow().getInt16(0).value(), 123);
	BOOST_CHECK(!message.getRow().getString(12).has_value());

	RequestMessageFormat pairFormat;
	pairFormat.addInt32(false);
	pairFormat.addString(true, 6);
	auto pairRequest = makeSetterRequest(attachment, pairFormat);
	auto& pairMessage = pairRequest.getMessage(0);

	struct Parameters
	{
		std::int32_t id;
		std::optional<std::string_view> name;
	};

	pairMessage.set(Parameters{42, "agg"});
	BOOST_CHECK_EQUAL(pairMessage.getRow().getInt32(0).value(), 42);
	BOOST_CHECK_EQUAL(pairMessage.getRow().getString(1).value(), "agg");

	pairMessage.set(std::tuple<std::int32_t, std::optional<std::string_view>>{43, "tuple"});
	BOOST_CHECK_EQUAL(pairMessage.getRow().getInt32(0).value(), 43);
	BOOST_CHECK_EQUAL(pairMessage.getRow().getString(1).value(), "tuple");

	pairMessage.set(0, std::variant<std::monostate, std::int32_t>{44});
	pairMessage.set(1, std::variant<std::monostate, std::string>{std::string{"var"}});
	BOOST_CHECK_EQUAL(pairMessage.getRow().getInt32(0).value(), 44);
	BOOST_CHECK_EQUAL(pairMessage.getRow().getString(1).value(), "var");

	pairMessage.set(1, std::variant<std::monostate, std::string>{std::monostate{}});
	BOOST_CHECK(!pairMessage.getRow().getString(1).has_value());
	pairMessage.clearParameters();
	BOOST_CHECK_EQUAL(pairMessage.getRow().getInt32(0).value(), 44);
}

BOOST_AUTO_TEST_CASE(unwindRunningRequest)
{
	const auto database = getTempFile("Request-unwindRunningRequest.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};
	Transaction transaction{attachment};

	RequestMessageFormat outFormat;
	outFormat.addInt32(false);

	std::vector<std::uint8_t> blr{blr_version5, blr_begin};
	appendClause(blr, outFormat.buildBlrMessageClause(1));
	appendByte(blr, blr_send);
	appendByte(blr, 1u);
	appendByte(blr, blr_begin);
	appendByte(blr, 1u);
	appendByte(blr, blr_literal);
	appendByte(blr, blr_long);
	appendByte(blr, 0u);
	for (const auto b : {1u, 0u, 0u, 0u})
		appendByte(blr, static_cast<std::uint8_t>(b));
	appendByte(blr, blr_parameter);
	appendByte(blr, 1u);
	appendWord(blr, 0u);
	appendByte(blr, blr_end);
	appendByte(blr, blr_end);
	appendByte(blr, blr_eoc);

	Request request{attachment, toBytes(blr), {{1, outFormat}}};

	request.start(transaction);
	request.unwind();

	// The record already produced before the unwind stays deliverable.
	BOOST_CHECK(request.receive(1));
	BOOST_CHECK(!request.receive(1));
}

BOOST_AUTO_TEST_CASE(moveSemantics)
{
	const auto database = getTempFile("Request-moveSemantics.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};
	Transaction transaction{attachment};

	RequestMessageFormat format;
	format.addInt32(false);

	const auto buildRequest = [&]()
	{
		std::vector<std::uint8_t> blr{blr_version5, blr_begin};
		appendClause(blr, format.buildBlrMessageClause(1));
		appendByte(blr, blr_send);
		appendByte(blr, 1u);
		appendByte(blr, blr_begin);
		appendByte(blr, 1u);
		appendByte(blr, blr_literal);
		appendByte(blr, blr_long);
		appendByte(blr, 0u);
		for (const auto b : {9u, 0u, 0u, 0u})
			appendByte(blr, static_cast<std::uint8_t>(b));
		appendByte(blr, blr_parameter);
		appendByte(blr, 1u);
		appendWord(blr, 0u);
		appendByte(blr, blr_end);
		appendByte(blr, blr_end);
		appendByte(blr, blr_eoc);
		return toBytes(blr);
	};

	Request request1{attachment, buildRequest(), {{1, format}}};
	BOOST_CHECK(request1.isValid());

	Request request2{std::move(request1)};
	BOOST_CHECK(!request1.isValid());
	BOOST_CHECK(request2.isValid());

	request2.start(transaction);
	BOOST_CHECK(request2.receive(1));
	BOOST_CHECK_EQUAL(request2.getMessage(1).getRow().getInt32(0).value(), 9);

	Request request3{attachment, buildRequest(), {{1, format}}};
	request2 = std::move(request3);
	BOOST_CHECK(!request3.isValid());
	BOOST_CHECK(request2.isValid());

	request2.start(transaction);
	BOOST_CHECK(request2.receive(1));
	BOOST_CHECK_EQUAL(request2.getMessage(1).getRow().getInt32(0).value(), 9);
}

BOOST_AUTO_TEST_SUITE_END()
