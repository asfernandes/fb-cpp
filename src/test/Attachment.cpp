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
#include "fb-cpp/Attachment.h"
#include "fb-cpp/Exception.h"
#include "fb-cpp/RowSet.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <exception>


BOOST_AUTO_TEST_SUITE(AttachmentSuite)

BOOST_AUTO_TEST_CASE(constructor)
{
	const auto database = getTempFile("Attachment-constructor.fdb");
	Attachment attachment1{CLIENT, database,
		AttachmentOptions().setCreateDatabase(true).setForcedWrites(false).setConnectionCharSet("UTF8")};
	attachment1.disconnect();

	Attachment attachment2{CLIENT, database};
	attachment2.dropDatabase();
}

BOOST_AUTO_TEST_CASE(disconnect)
{
	const auto database = getTempFile("Attachment-disconnect.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	attachment1.disconnect();

	Attachment attachment2{CLIENT, database, AttachmentOptions().setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment2};
}

BOOST_AUTO_TEST_CASE(dropDatabase)
{
	const auto database = getTempFile("Attachment-dropDatabase.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	attachment1.dropDatabase();

	BOOST_CHECK_THROW(Attachment(CLIENT, database), DatabaseException);
}

BOOST_AUTO_TEST_CASE(sqlDialectSetterGetter)
{
	AttachmentOptions options;
	BOOST_CHECK(!options.getSqlDialect().has_value());

	options.setSqlDialect(1u);
	BOOST_REQUIRE(options.getSqlDialect().has_value());
	BOOST_CHECK_EQUAL(*options.getSqlDialect(), 1u);
}

BOOST_AUTO_TEST_CASE(forcedWritesDefault)
{
	AttachmentOptions options;
	BOOST_CHECK(!options.getForcedWrites().has_value());
}

BOOST_AUTO_TEST_CASE(createDatabaseWithForcedWritesOff)
{
	const auto database = getTempFile("Attachment-createDatabaseWithForcedWritesOff.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement stmt{attachment, transaction, "select mon$forced_writes from mon$database"};
	BOOST_REQUIRE(stmt.execute(transaction));
	BOOST_REQUIRE(stmt.getInt32(0).has_value());
	BOOST_CHECK_EQUAL(*stmt.getInt32(0), 0);
	transaction.commit();
}

BOOST_AUTO_TEST_CASE(executePreparesAndExecutesStatement)
{
	const auto database = getTempFile("Attachment-executePreparesAndExecutesStatement.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK(attachment.execute(transaction, "create table t (id integer not null primary key)"));
	transaction.commitRetaining();

	BOOST_CHECK(attachment.execute(transaction, "insert into t (id) values (1)"));
	BOOST_CHECK(attachment.execute(transaction, "select id from t"));
	BOOST_CHECK(!attachment.execute(transaction, "select id from t where id = 2"));

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryReturnsRowsIncludingFirstRow)
{
	const auto database = getTempFile("Attachment-queryReturnsRowsIncludingFirstRow.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction, "create table t (id integer not null primary key)"));
	transaction.commitRetaining();

	for (int i = 1; i <= 3; ++i)
	{
		Statement insert{attachment, transaction, "insert into t (id) values (?)"};
		insert.setInt32(0, i);
		BOOST_REQUIRE(insert.execute(transaction));
	}

	auto rowSet = attachment.queryRowSet(transaction, "select id from t order by id", 10u);

	BOOST_REQUIRE_EQUAL(rowSet.getCount(), 3u);
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(rowSet.getRow(1).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(rowSet.getRow(2).getInt32(0).value(), 3);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryHonorsMaxRows)
{
	const auto database = getTempFile("Attachment-queryHonorsMaxRows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction, "create table t (id integer not null primary key)"));
	transaction.commitRetaining();

	for (int i = 1; i <= 5; ++i)
	{
		Statement insert{attachment, transaction, "insert into t (id) values (?)"};
		insert.setInt32(0, i);
		BOOST_REQUIRE(insert.execute(transaction));
	}

	auto rowSet = attachment.queryRowSet(transaction, "select id from t order by id", 2u);

	BOOST_REQUIRE_EQUAL(rowSet.getCount(), 2u);
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(rowSet.getRow(1).getInt32(0).value(), 2);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryReturnsEmptyRowSetForNoRows)
{
	const auto database = getTempFile("Attachment-queryReturnsEmptyRowSetForNoRows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction, "create table t (id integer not null primary key)"));
	transaction.commitRetaining();

	auto rowSet = attachment.queryRowSet(transaction, "select id from t", 10u);

	BOOST_CHECK_EQUAL(rowSet.getCount(), 0u);
	BOOST_CHECK(rowSet.getRawBuffer().empty());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(querySupportsStatementOptions)
{
	const auto database = getTempFile("Attachment-querySupportsStatementOptions.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	auto rowSet =
		attachment.queryRowSet(transaction, "select 1 from rdb$database", 1u, StatementOptions().setDialect(3u));

	BOOST_REQUIRE_EQUAL(rowSet.getCount(), 1u);
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 1);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryThrowsForNonQueryStatement)
{
	const auto database = getTempFile("Attachment-queryThrowsForNonQueryStatement.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK_THROW(attachment.queryRowSet(transaction, "create table t (id integer)", 10u), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryRowSetSupportsProcedureWithOutputColumns)
{
	const auto database = getTempFile("Attachment-queryRowSetSupportsProcedureWithOutputColumns.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction,
		"create procedure p returns (id integer, name varchar(20)) as begin id = 42; name = 'answer'; suspend; end"));
	transaction.commitRetaining();

	auto rowSet = attachment.queryRowSet(transaction, "execute procedure p", 10u);

	BOOST_REQUIRE_EQUAL(rowSet.getCount(), 1u);
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 42);
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getString(1).value(), "answer");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryRowSetRejectsProcedureWithoutOutputColumns)
{
	const auto database = getTempFile("Attachment-queryRowSetRejectsProcedureWithoutOutputColumns.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction, "create procedure p as begin end"));
	transaction.commitRetaining();

	BOOST_CHECK_THROW(attachment.queryRowSet(transaction, "execute procedure p", 10u), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(isNotValidAfterMove)
{
	const auto database = getTempFile("Attachment-isNotValidAfterMove.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	BOOST_CHECK_EQUAL(attachment1.isValid(), true);

	auto attachment2 = std::move(attachment1);
	FbDropDatabase attachmentDrop{attachment2};
	BOOST_CHECK_EQUAL(attachment2.isValid(), true);
	BOOST_CHECK_EQUAL(attachment1.isValid(), false);
}

BOOST_AUTO_TEST_CASE(moveAssignmentTransfersOwnership)
{
	const auto database1 = getTempFile("Attachment-moveAssign-1.fdb");
	const auto database2 = getTempFile("Attachment-moveAssign-2.fdb");

	Attachment attachment1{CLIENT, database1, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	Attachment attachment2{CLIENT, database2, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	BOOST_CHECK(attachment1.isValid());
	BOOST_CHECK(attachment2.isValid());

	// Move-assign attachment2 into attachment1.
	// attachment1's old connection is disconnected; attachment2 becomes invalid.
	attachment1 = std::move(attachment2);
	BOOST_CHECK(attachment1.isValid());
	BOOST_CHECK(!attachment2.isValid());

	// The moved-to attachment can still operate on the database.
	attachment1.dropDatabase();
	BOOST_CHECK(!attachment1.isValid());

	// Clean up the first database (its connection was disconnected by the move).
	Attachment cleanup{CLIENT, database1};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_CASE(isNotValidAfterDisconnect)
{
	const auto database = getTempFile("Attachment-isNotValidAfterDisconnect.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	BOOST_CHECK_EQUAL(attachment1.isValid(), true);

	attachment1.disconnect();
	BOOST_CHECK_EQUAL(attachment1.isValid(), false);

	Attachment attachment2{CLIENT, database};
	attachment2.dropDatabase();
}

BOOST_AUTO_TEST_CASE(isNotValidAfterDropDatabase)
{
	Attachment attachment1{CLIENT, getTempFile("Attachment-isNotValidAfterDropDatabase.fdb"),
		AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	BOOST_CHECK_EQUAL(attachment1.isValid(), true);

	attachment1.dropDatabase();
	BOOST_CHECK_EQUAL(attachment1.isValid(), false);
}

BOOST_AUTO_TEST_SUITE_END()
