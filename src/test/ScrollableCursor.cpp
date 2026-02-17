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
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"


BOOST_AUTO_TEST_SUITE(ScrollableCursorSuite)

BOOST_AUTO_TEST_CASE(defaultCursorTypeIsForwardOnly)
{
	StatementOptions options;
	BOOST_CHECK(options.getCursorType() == CursorType::FORWARD_ONLY);
}

BOOST_AUTO_TEST_CASE(scrollableCursorSupportsFetchFirst)
{
	const auto database = getTempFile("ScrollableCursor-fetchFirst.fdb");

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

	Statement select{attachment, transaction, "select col from t order by col",
		StatementOptions().setCursorType(CursorType::SCROLLABLE)};
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);

	BOOST_REQUIRE(select.fetchNext());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 2);

	BOOST_REQUIRE(select.fetchFirst());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);
}

BOOST_AUTO_TEST_CASE(scrollableCursorSupportsFetchLast)
{
	const auto database = getTempFile("ScrollableCursor-fetchLast.fdb");

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

	Statement select{attachment, transaction, "select col from t order by col",
		StatementOptions().setCursorType(CursorType::SCROLLABLE)};
	BOOST_REQUIRE(select.execute(transaction));

	BOOST_REQUIRE(select.fetchLast());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 3);
}

BOOST_AUTO_TEST_CASE(scrollableCursorSupportsFetchPrior)
{
	const auto database = getTempFile("ScrollableCursor-fetchPrior.fdb");

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

	Statement select{attachment, transaction, "select col from t order by col",
		StatementOptions().setCursorType(CursorType::SCROLLABLE)};
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);

	BOOST_REQUIRE(select.fetchNext());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 2);

	BOOST_REQUIRE(select.fetchPrior());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);
}

BOOST_AUTO_TEST_CASE(scrollableCursorSupportsFetchAbsolute)
{
	const auto database = getTempFile("ScrollableCursor-fetchAbsolute.fdb");

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

	Statement select{attachment, transaction, "select col from t order by col",
		StatementOptions().setCursorType(CursorType::SCROLLABLE)};
	BOOST_REQUIRE(select.execute(transaction));

	BOOST_REQUIRE(select.fetchAbsolute(3));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 3);

	BOOST_REQUIRE(select.fetchAbsolute(1));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);

	BOOST_REQUIRE(select.fetchAbsolute(5));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 5);
}

BOOST_AUTO_TEST_CASE(scrollableCursorSupportsFetchRelative)
{
	const auto database = getTempFile("ScrollableCursor-fetchRelative.fdb");

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

	Statement select{attachment, transaction, "select col from t order by col",
		StatementOptions().setCursorType(CursorType::SCROLLABLE)};
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);

	BOOST_REQUIRE(select.fetchRelative(2));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 3);

	BOOST_REQUIRE(select.fetchRelative(-1));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 2);
}

BOOST_AUTO_TEST_CASE(forwardOnlyIsDefault)
{
	const auto database = getTempFile("ScrollableCursor-forwardOnlyIsDefault.fdb");

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

	// Default options — forward-only cursor
	Statement select{attachment, transaction, "select col from t order by col"};
	BOOST_REQUIRE(select.execute(transaction));
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);

	BOOST_REQUIRE(select.fetchNext());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 2);

	BOOST_REQUIRE(select.fetchNext());
	BOOST_CHECK_EQUAL(select.getInt32(0).value(), 3);

	BOOST_CHECK_EQUAL(select.fetchNext(), false);
}

BOOST_AUTO_TEST_SUITE_END()
