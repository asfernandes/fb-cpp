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


BOOST_AUTO_TEST_SUITE(RowSetSuite)

BOOST_AUTO_TEST_CASE(fetchRowsIntoRowSet)
{
	const auto database = getTempFile("RowSet-fetchRowsIntoRowSet.fdb");

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

	// The first row (1) was fetched by execute(). Fetch remaining rows into RowSet.
	RowSet rowSet{select, 10};

	BOOST_CHECK_EQUAL(rowSet.getCount(), 4u);
	BOOST_CHECK(rowSet.getMessageLength() > 0);
	BOOST_CHECK_EQUAL(
		rowSet.getRawBuffer().size(), static_cast<std::size_t>(rowSet.getCount()) * rowSet.getMessageLength());

	// Verify row data using typed Row access.
	for (unsigned i = 0; i < rowSet.getCount(); ++i)
	{
		auto row = rowSet.getRow(i);
		BOOST_CHECK_EQUAL(row.getInt32(0).value(), static_cast<std::int32_t>(i + 2));
	}
}

BOOST_AUTO_TEST_CASE(fetchFewerRowsThanMaxRows)
{
	const auto database = getTempFile("RowSet-fetchFewerRowsThanMaxRows.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
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

	// execute() fetched row 1. Request 100 rows but only 2 remain.
	RowSet rowSet{select, 100};

	BOOST_CHECK_EQUAL(rowSet.getCount(), 2u);
	BOOST_CHECK_EQUAL(
		rowSet.getRawBuffer().size(), static_cast<std::size_t>(rowSet.getCount()) * rowSet.getMessageLength());
}

BOOST_AUTO_TEST_CASE(rowSetIsDisconnectedFromStatement)
{
	const auto database = getTempFile("RowSet-isDisconnectedFromStatement.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
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
	BOOST_CHECK_EQUAL(rowSet.getCount(), 2u);

	// Free the statement; the RowSet data is still valid.
	select.free();

	BOOST_CHECK(!rowSet.getRawBuffer().empty());
	BOOST_CHECK_EQUAL(rowSet.getCount(), 2u);

	// Typed access still works after the statement is freed.
	BOOST_CHECK_EQUAL(rowSet.getRow(0).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(rowSet.getRow(1).getInt32(0).value(), 3);
}

BOOST_AUTO_TEST_CASE(moveConstructor)
{
	const auto database = getTempFile("RowSet-moveConstructor.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
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
}

BOOST_AUTO_TEST_CASE(fetchMultipleBatchesFromSameStatement)
{
	const auto database = getTempFile("RowSet-fetchMultipleBatches.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
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

	// execute() fetched row 1. Fetch batches of 3 from the remaining 9 rows.
	RowSet batch1{select, 3};
	BOOST_CHECK_EQUAL(batch1.getCount(), 3u);
	BOOST_CHECK_EQUAL(batch1.getRow(0).getInt32(0).value(), 2);
	BOOST_CHECK_EQUAL(batch1.getRow(1).getInt32(0).value(), 3);
	BOOST_CHECK_EQUAL(batch1.getRow(2).getInt32(0).value(), 4);

	RowSet batch2{select, 3};
	BOOST_CHECK_EQUAL(batch2.getCount(), 3u);
	BOOST_CHECK_EQUAL(batch2.getRow(0).getInt32(0).value(), 5);
	BOOST_CHECK_EQUAL(batch2.getRow(1).getInt32(0).value(), 6);
	BOOST_CHECK_EQUAL(batch2.getRow(2).getInt32(0).value(), 7);

	RowSet batch3{select, 3};
	BOOST_CHECK_EQUAL(batch3.getCount(), 3u);
	BOOST_CHECK_EQUAL(batch3.getRow(0).getInt32(0).value(), 8);
	BOOST_CHECK_EQUAL(batch3.getRow(1).getInt32(0).value(), 9);
	BOOST_CHECK_EQUAL(batch3.getRow(2).getInt32(0).value(), 10);

	// No more rows; the next batch should be empty.
	RowSet batch4{select, 3};
	BOOST_CHECK_EQUAL(batch4.getCount(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
