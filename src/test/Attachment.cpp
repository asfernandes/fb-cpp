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

BOOST_AUTO_TEST_CASE(queryScalarReturnsFirstColumnOfFirstRow)
{
	const auto database = getTempFile("Attachment-queryScalarReturnsFirstColumnOfFirstRow.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(
		attachment.execute(transaction, "create table t (id integer not null primary key, name varchar(20))"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id, name) values (1, 'one')"));
	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id, name) values (2, 'two')"));

	const auto value = attachment.queryScalar<std::string>(transaction, "select name, id from t order by id");

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(*value, "one");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryScalarReturnsNulloptForNoRows)
{
	const auto database = getTempFile("Attachment-queryScalarReturnsNulloptForNoRows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	const auto value = attachment.queryScalar<std::int32_t>(transaction, "select 1 from rdb$database where 1 = 0");

	BOOST_CHECK(!value.has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryScalarReturnsNulloptForNullColumn)
{
	const auto database = getTempFile("Attachment-queryScalarReturnsNulloptForNullColumn.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	const auto value =
		attachment.queryScalar<std::int32_t>(transaction, "select cast(null as integer) from rdb$database");

	BOOST_CHECK(!value.has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryScalarSupportsStatementOptions)
{
	const auto database = getTempFile("Attachment-queryScalarSupportsStatementOptions.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	const auto value = attachment.queryScalar<std::int32_t>(
		transaction, "select 1 from rdb$database", StatementOptions().setDialect(3u));

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(*value, 1);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryScalarSupportsProcedureWithOutputColumns)
{
	const auto database = getTempFile("Attachment-queryScalarSupportsProcedureWithOutputColumns.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction,
		"create procedure p returns (id integer, name varchar(20)) as begin id = 42; name = 'answer'; suspend; end"));
	transaction.commitRetaining();

	const auto value = attachment.queryScalar<std::int32_t>(transaction, "execute procedure p");

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(*value, 42);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryScalarThrowsForNonQueryStatement)
{
	const auto database = getTempFile("Attachment-queryScalarThrowsForNonQueryStatement.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK_THROW(attachment.queryScalar<std::int32_t>(transaction, "create table t (id integer)"), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsReturnsStructFromFirstRow)
{
	struct Result
	{
		std::int32_t id;
		std::optional<std::string> name;
	};

	const auto database = getTempFile("Attachment-queryFirstRowAsReturnsStructFromFirstRow.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(
		attachment.execute(transaction, "create table t (id integer not null primary key, name varchar(20))"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id, name) values (1, 'one')"));
	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id, name) values (2, 'two')"));

	const auto value = attachment.queryFirstRowAs<Result>(transaction, "select id, name from t order by id");

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(value->id, 1);
	BOOST_REQUIRE(value->name.has_value());
	BOOST_CHECK_EQUAL(*value->name, "one");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsReturnsTupleFromFirstRow)
{
	using Result = std::tuple<std::int32_t, std::optional<std::string>>;

	const auto database = getTempFile("Attachment-queryFirstRowAsReturnsTupleFromFirstRow.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	const auto value = attachment.queryFirstRowAs<Result>(transaction, "select 42, 'answer' from rdb$database");

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(std::get<0>(*value), 42);
	BOOST_REQUIRE(std::get<1>(*value).has_value());
	BOOST_CHECK_EQUAL(*std::get<1>(*value), "answer");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsReturnsNulloptForNoRows)
{
	struct Result
	{
		std::int32_t id;
	};

	const auto database = getTempFile("Attachment-queryFirstRowAsReturnsNulloptForNoRows.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	const auto value = attachment.queryFirstRowAs<Result>(transaction, "select 1 from rdb$database where 1 = 0");

	BOOST_CHECK(!value.has_value());

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsSupportsStatementOptions)
{
	using Result = std::tuple<std::int32_t>;

	const auto database = getTempFile("Attachment-queryFirstRowAsSupportsStatementOptions.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	const auto value = attachment.queryFirstRowAs<Result>(
		transaction, "select 1 from rdb$database", StatementOptions().setDialect(3u));

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(std::get<0>(*value), 1);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsSupportsProcedureWithOutputColumns)
{
	struct Result
	{
		std::int32_t id;
		std::optional<std::string> name;
	};

	const auto database = getTempFile("Attachment-queryFirstRowAsSupportsProcedureWithOutputColumns.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction,
		"create procedure p returns (id integer, name varchar(20)) as begin id = 42; name = 'answer'; suspend; end"));
	transaction.commitRetaining();

	const auto value = attachment.queryFirstRowAs<Result>(transaction, "execute procedure p");

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(value->id, 42);
	BOOST_REQUIRE(value->name.has_value());
	BOOST_CHECK_EQUAL(*value->name, "answer");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsThrowsForFieldCountMismatch)
{
	struct Result
	{
		std::int32_t id;
	};

	const auto database = getTempFile("Attachment-queryFirstRowAsThrowsForFieldCountMismatch.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK_THROW(attachment.queryFirstRowAs<Result>(transaction, "select 1, 2 from rdb$database"), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsThrowsForNullIntoNonOptionalField)
{
	struct Result
	{
		std::int32_t id;
	};

	const auto database = getTempFile("Attachment-queryFirstRowAsThrowsForNullIntoNonOptionalField.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK_THROW(attachment.queryFirstRowAs<Result>(transaction, "select cast(null as integer) from rdb$database"),
		FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsThrowsForNonQueryStatement)
{
	struct Result
	{
		std::int32_t id;
	};

	const auto database = getTempFile("Attachment-queryFirstRowAsThrowsForNonQueryStatement.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK_THROW(attachment.queryFirstRowAs<Result>(transaction, "create table t (id integer)"), FbCppException);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(executeBindsTupleParameters)
{
	const auto database = getTempFile("Attachment-executeBindsTupleParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(
		attachment.execute(transaction, "create table t (id integer not null primary key, name varchar(20))"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(
		transaction, "insert into t (id, name) values (?, ?)", std::tuple{1, std::string_view{"one"}}));
	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id, name) values (?, ?)",
		StatementOptions().setDialect(3u), std::tuple{2, std::string_view{"two"}}));

	const auto count = attachment.queryScalar<std::int32_t>(transaction, "select count(*) from t");
	BOOST_REQUIRE(count.has_value());
	BOOST_CHECK_EQUAL(*count, 2);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(executeBindsStructParameters)
{
	struct Params
	{
		std::int32_t id;
		std::string_view name;
	};

	const auto database = getTempFile("Attachment-executeBindsStructParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(
		attachment.execute(transaction, "create table t (id integer not null primary key, name varchar(20))"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id, name) values (?, ?)", Params{1, "one"}));

	const auto name = attachment.queryScalar<std::string>(transaction, "select name from t where id = 1");
	BOOST_REQUIRE(name.has_value());
	BOOST_CHECK_EQUAL(*name, "one");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryRowSetBindsTupleParameters)
{
	const auto database = getTempFile("Attachment-queryRowSetBindsTupleParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction, "create table t (id integer not null primary key)"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id) values (?)", std::tuple{1}));
	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id) values (?)", std::tuple{2}));
	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id) values (?)", std::tuple{3}));

	auto rowSet = attachment.queryRowSet(transaction, "select id from t where id > ? order by id", 10u, std::tuple{1});

	BOOST_REQUIRE_EQUAL(rowSet.getCount(), 2u);
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(rowSet.getRow(1).getInt32(0).value(), 3);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryRowSetBindsOptionsAndStructParameters)
{
	struct Params
	{
		std::int32_t minId;
	};

	const auto database = getTempFile("Attachment-queryRowSetBindsOptionsAndStructParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(attachment.execute(transaction, "create table t (id integer not null primary key)"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id) values (?)", std::tuple{1}));
	BOOST_REQUIRE(attachment.execute(transaction, "insert into t (id) values (?)", std::tuple{2}));

	auto rowSet = attachment.queryRowSet(
		transaction, "select id from t where id > ? order by id", 10u, StatementOptions().setDialect(3u), Params{1});

	BOOST_REQUIRE_EQUAL(rowSet.getCount(), 1u);
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 2);

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryScalarBindsTupleParameters)
{
	const auto database = getTempFile("Attachment-queryScalarBindsTupleParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(
		attachment.execute(transaction, "create table t (id integer not null primary key, name varchar(20))"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(
		transaction, "insert into t (id, name) values (?, ?)", std::tuple{1, std::string_view{"one"}}));

	const auto name =
		attachment.queryScalar<std::string>(transaction, "select name from t where id = ?", std::tuple{1});
	const auto nameWithOptions = attachment.queryScalar<std::string>(
		transaction, "select name from t where id = ?", StatementOptions().setDialect(3u), std::tuple{1});

	BOOST_REQUIRE(name.has_value());
	BOOST_CHECK_EQUAL(*name, "one");
	BOOST_REQUIRE(nameWithOptions.has_value());
	BOOST_CHECK_EQUAL(*nameWithOptions, "one");

	transaction.commit();
}

BOOST_AUTO_TEST_CASE(queryFirstRowAsBindsStructParameters)
{
	struct Params
	{
		std::int32_t id;
	};

	struct Result
	{
		std::int32_t id;
		std::optional<std::string> name;
	};

	const auto database = getTempFile("Attachment-queryFirstRowAsBindsStructParameters.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_REQUIRE(
		attachment.execute(transaction, "create table t (id integer not null primary key, name varchar(20))"));
	transaction.commitRetaining();

	BOOST_REQUIRE(attachment.execute(
		transaction, "insert into t (id, name) values (?, ?)", std::tuple{1, std::string_view{"one"}}));

	const auto value =
		attachment.queryFirstRowAs<Result>(transaction, "select id, name from t where id = ?", Params{1});
	const auto valueWithOptions = attachment.queryFirstRowAs<Result>(
		transaction, "select id, name from t where id = ?", StatementOptions().setDialect(3u), Params{1});

	BOOST_REQUIRE(value.has_value());
	BOOST_CHECK_EQUAL(value->id, 1);
	BOOST_REQUIRE(value->name.has_value());
	BOOST_CHECK_EQUAL(*value->name, "one");
	BOOST_REQUIRE(valueWithOptions.has_value());
	BOOST_CHECK_EQUAL(valueWithOptions->id, 1);

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

BOOST_AUTO_TEST_CASE(ping)
{
	const auto database = getTempFile("Attachment-ping.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	BOOST_CHECK_NO_THROW(attachment.ping());
}

BOOST_AUTO_TEST_CASE(resetSession)
{
	const auto database = getTempFile("Attachment-resetSession.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	attachment.execute(transaction,
		"select rdb$set_context('USER_SESSION', 'test_var', 'test_value') from rdb$database");
	const auto valueBefore = attachment.queryScalar<std::string>(
		transaction, "select rdb$get_context('USER_SESSION', 'test_var') from rdb$database");
	BOOST_REQUIRE(valueBefore.has_value());
	BOOST_CHECK_EQUAL(*valueBefore, "test_value");
	transaction.commit();

	attachment.resetSession();

	Transaction transaction2{attachment};
	const auto valueAfter = attachment.queryScalar<std::string>(
		transaction2, "select rdb$get_context('USER_SESSION', 'test_var') from rdb$database");
	BOOST_CHECK(!valueAfter.has_value());
	transaction2.commit();
}

BOOST_AUTO_TEST_SUITE_END()
