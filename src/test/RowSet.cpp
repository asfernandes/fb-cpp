/*
 * MIT License
 *
 * Copyright (c) 2026 F.D.Castel
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
#include "fb-cpp/RowSet.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <cstddef>
#include <vector>


BOOST_AUTO_TEST_SUITE(RowSetSuite)

BOOST_AUTO_TEST_CASE(fetchRowsIntoRowSet)
{
	const auto database = getTempFile("RowSet-fetchRowsIntoRowSet.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true)};
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
	BOOST_CHECK(select.hasCurrentRow());

	RowSet rowSet{select, 10};

	BOOST_CHECK(!select.hasCurrentRow());
	BOOST_CHECK_EQUAL(rowSet.getCount(), 5u);
	BOOST_CHECK(rowSet.getMessageLength() > 0);
	BOOST_CHECK_EQUAL(
		rowSet.getRawBuffer().size(), static_cast<std::size_t>(rowSet.getCount()) * rowSet.getMessageLength());

	// Verify row data using typed Row access. The current execute() row is included.
	for (unsigned i = 0; i < rowSet.getCount(); ++i)
	{
		auto row = rowSet.getRow(i);
		BOOST_CHECK_EQUAL(row.getInt32(0).value(), static_cast<std::int32_t>(i + 1));
	}
}

BOOST_AUTO_TEST_CASE(fetchFewerRowsThanMaxRows)
{
	const auto database = getTempFile("RowSet-fetchFewerRowsThanMaxRows.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddl{attachment, transaction, "create table t (col integer)"};
	ddl.execute(transaction);
	transaction.commitRetaining();

	Statement insert{attachment, transaction, "insert into t (col) values (?)"};
	for (int i = 1; i <= 3; ++i)
	{
		insert.setInt32(0, i);
		insert.execute(transaction);
	}

	Statement select{attachment, transaction, "select col from t order by col"};
	BOOST_REQUIRE(select.execute(transaction));

	// Request more rows than exist; all 3 rows are returned.
	RowSet rowSet{select, 100};

	BOOST_CHECK_EQUAL(rowSet.getCount(), 3u);
	BOOST_CHECK_EQUAL(
		rowSet.getRawBuffer().size(), static_cast<std::size_t>(rowSet.getCount()) * rowSet.getMessageLength());
}

BOOST_AUTO_TEST_CASE(rowSetIsDisconnectedFromStatement)
{
	const auto database = getTempFile("RowSet-isDisconnectedFromStatement.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddl{attachment, transaction, "create table t (col integer)"};
	ddl.execute(transaction);
	transaction.commitRetaining();

	Statement insert{attachment, transaction, "insert into t (col) values (?)"};
	for (int i = 1; i <= 3; ++i)
	{
		insert.setInt32(0, i);
		insert.execute(transaction);
	}

	Statement select{attachment, transaction, "select col from t order by col"};
	BOOST_REQUIRE(select.execute(transaction));

	RowSet rowSet{select, 10};
	BOOST_CHECK_EQUAL(rowSet.getCount(), 3u);

	// Free the statement; the RowSet data is still valid.
	select.free();

	BOOST_CHECK(!rowSet.getRawBuffer().empty());
	BOOST_CHECK_EQUAL(rowSet.getCount(), 3u);

	// Typed access still works after the statement is freed.
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(rowSet.getRow(1).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(rowSet.getRow(2).getInt32(0).value(), 3);
}

BOOST_AUTO_TEST_CASE(moveConstructor)
{
	const auto database = getTempFile("RowSet-moveConstructor.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddl{attachment, transaction, "create table t (col integer)"};
	ddl.execute(transaction);
	transaction.commitRetaining();

	Statement insert{attachment, transaction, "insert into t (col) values (?)"};
	for (int i = 1; i <= 3; ++i)
	{
		insert.setInt32(0, i);
		insert.execute(transaction);
	}

	Statement select{attachment, transaction, "select col from t order by col"};
	BOOST_REQUIRE(select.execute(transaction));

	RowSet rowSet1{select, 10};
	const auto count = rowSet1.getCount();

	RowSet rowSet2{std::move(rowSet1)};
	BOOST_CHECK_EQUAL(rowSet2.getCount(), count);
	BOOST_CHECK_EQUAL(rowSet1.getCount(), 0u);

	// Typed access works on the moved-to RowSet.
	BOOST_CHECK_EQUAL(rowSet2.getRow(0).getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(rowSet2.getRow(1).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(rowSet2.getRow(2).getInt32(0).value(), 3);
}

BOOST_AUTO_TEST_CASE(moveAssignment)
{
	const auto database = getTempFile("RowSet-moveAssignment.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true)};
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

	// Fetch rows 1-2 into first batch, rows 3-4 into second.
	RowSet rowSet1{select, 2};
	RowSet rowSet2{select, 2};

	BOOST_CHECK_EQUAL(rowSet1.getCount(), 2u);
	BOOST_CHECK_EQUAL(rowSet1.getRow(0).getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(rowSet2.getCount(), 2u);
	BOOST_CHECK_EQUAL(rowSet2.getRow(0).getInt32(0).value(), 3);

	// Move-assign rowSet2 into rowSet1 (overwrites old data).
	rowSet1 = std::move(rowSet2);

	BOOST_CHECK_EQUAL(rowSet1.getCount(), 2u);
	BOOST_CHECK_EQUAL(rowSet1.getRow(0).getInt32(0).value(), 3);
	BOOST_CHECK_EQUAL(rowSet1.getRow(1).getInt32(0).value(), 4);
	BOOST_CHECK_EQUAL(rowSet2.getCount(), 0u);
}

BOOST_AUTO_TEST_CASE(fetchMultipleBatchesFromSameStatement)
{
	const auto database = getTempFile("RowSet-fetchMultipleBatches.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddl{attachment, transaction, "create table t (col integer)"};
	ddl.execute(transaction);
	transaction.commitRetaining();

	Statement insert{attachment, transaction, "insert into t (col) values (?)"};
	for (int i = 1; i <= 10; ++i)
	{
		insert.setInt32(0, i);
		insert.execute(transaction);
	}

	Statement select{attachment, transaction, "select col from t order by col"};
	BOOST_REQUIRE(select.execute(transaction));

	// Fetch batches of 3, including the current execute() row.
	RowSet batch1{select, 3};
	BOOST_CHECK_EQUAL(batch1.getCount(), 3u);
	BOOST_CHECK_EQUAL(batch1.getRow(0).getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(batch1.getRow(1).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(batch1.getRow(2).getInt32(0).value(), 3);

	RowSet batch2{select, 3};
	BOOST_CHECK_EQUAL(batch2.getCount(), 3u);
	BOOST_CHECK_EQUAL(batch2.getRow(0).getInt32(0).value(), 4);
	BOOST_CHECK_EQUAL(batch2.getRow(1).getInt32(0).value(), 5);
	BOOST_CHECK_EQUAL(batch2.getRow(2).getInt32(0).value(), 6);

	RowSet batch3{select, 3};
	BOOST_CHECK_EQUAL(batch3.getCount(), 3u);
	BOOST_CHECK_EQUAL(batch3.getRow(0).getInt32(0).value(), 7);
	BOOST_CHECK_EQUAL(batch3.getRow(1).getInt32(0).value(), 8);
	BOOST_CHECK_EQUAL(batch3.getRow(2).getInt32(0).value(), 9);

	RowSet batch4{select, 3};
	BOOST_CHECK_EQUAL(batch4.getCount(), 1u);
	BOOST_CHECK_EQUAL(batch4.getRow(0).getInt32(0).value(), 10);

	// No more rows; the next batch should be empty.
	RowSet batch5{select, 3};
	BOOST_CHECK_EQUAL(batch5.getCount(), 0u);
}

BOOST_AUTO_TEST_CASE(includesCurrentRowAndDoesNotDuplicateAcrossUses)
{
	const auto database = getTempFile("RowSet-includesCurrentRowAndDoesNotDuplicate.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	Statement ddl{attachment, transaction, "create table t (col integer)"};
	ddl.execute(transaction);
	transaction.commitRetaining();

	Statement insert{attachment, transaction, "insert into t (col) values (?)"};
	for (int i = 1; i <= 4; ++i)
	{
		insert.setInt32(0, i);
		insert.execute(transaction);
	}

	Statement emptySelect{attachment, transaction, "select col from t where col = 0"};
	BOOST_CHECK(!emptySelect.execute(transaction));
	BOOST_CHECK(!emptySelect.hasCurrentRow());

	RowSet emptyRowSet{emptySelect, 10};
	BOOST_CHECK_EQUAL(emptyRowSet.getCount(), 0u);
	BOOST_CHECK(emptyRowSet.getRawBuffer().empty());

	Statement select{attachment, transaction, "select col from t order by col"};
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK(select.hasCurrentRow());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);

	BOOST_REQUIRE(select.fetchNext());
	BOOST_CHECK(select.hasCurrentRow());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 2);

	// After fetchNext(), RowSet starts at the new current row and does not go back to row 1.
	RowSet fromCurrent{select, 10};
	BOOST_CHECK(!select.hasCurrentRow());
	BOOST_REQUIRE_EQUAL(fromCurrent.getCount(), 3u);
	BOOST_CHECK_EQUAL(fromCurrent.getRow(0).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(fromCurrent.getRow(1).getInt32(0).value(), 3);
	BOOST_CHECK_EQUAL(fromCurrent.getRow(2).getInt32(0).value(), 4);

	BOOST_REQUIRE(select.execute(transaction));
	RowSet firstBatch{select, 2};
	RowSet secondBatch{select, 2};
	BOOST_REQUIRE_EQUAL(firstBatch.getCount(), 2u);
	BOOST_CHECK_EQUAL(firstBatch.getRow(0).getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(firstBatch.getRow(1).getInt32(0).value(), 2);
	BOOST_REQUIRE_EQUAL(secondBatch.getCount(), 2u);
	BOOST_CHECK_EQUAL(secondBatch.getRow(0).getInt32(0).value(), 3);
	BOOST_CHECK_EQUAL(secondBatch.getRow(1).getInt32(0).value(), 4);

	Statement procedureDdl{
		attachment, transaction, "create procedure p returns (col integer) as begin col = 42; suspend; end"};
	procedureDdl.execute(transaction);
	transaction.commitRetaining();

	Statement procedure{attachment, transaction, "execute procedure p"};
	BOOST_REQUIRE(procedure.execute(transaction));
	BOOST_CHECK(procedure.hasCurrentRow());

	RowSet procedureRowSet{procedure, 10};
	BOOST_CHECK(!procedure.hasCurrentRow());
	BOOST_REQUIRE_EQUAL(procedureRowSet.getCount(), 1u);
	BOOST_CHECK_EQUAL(procedureRowSet.getRow(0).getInt32(0).value(), 42);
}

BOOST_AUTO_TEST_CASE(readBytesFromDisconnectedRowSet)
{
	const auto database = getTempFile("RowSet-readBytesFromDisconnectedRowSet.fdb");

	Attachment attachment{
		getClient(), database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Statement select{
		attachment, transaction, "select cast('row bytes' as varchar(16) character set octets) from rdb$database"};
	BOOST_REQUIRE(select.execute(transaction));

	RowSet rowSet{select, 1u};
	select.free();

	const std::vector<std::byte> expected{
		std::byte{'r'},
		std::byte{'o'},
		std::byte{'w'},
		std::byte{' '},
		std::byte{'b'},
		std::byte{'y'},
		std::byte{'t'},
		std::byte{'e'},
		std::byte{'s'},
	};
	const auto result = rowSet.getRow(0).getBytes(0);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(*result == expected);
	const auto typedResult = rowSet.getRow(0).get<std::optional<std::vector<std::byte>>>(0);
	BOOST_REQUIRE(typedResult.has_value());
	BOOST_CHECK(*typedResult == expected);
}

BOOST_AUTO_TEST_SUITE_END()
