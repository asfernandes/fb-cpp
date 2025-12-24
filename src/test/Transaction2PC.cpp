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
#include "fb-cpp/Transaction.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Exception.h"
#include <exception>
#include <string>


BOOST_AUTO_TEST_SUITE(Transaction2PCSuite)

static int countLimbo(Attachment& attachment)
{
	TransactionOptions options;
	options.setIsolationLevel(TransactionIsolationLevel::READ_COMMITTED);

	Transaction transaction{attachment, options};
	Statement statement{attachment, transaction, "select count(*) from rdb$transactions"};
	statement.execute(transaction);
	int count = statement.getInt32(0).value();
	transaction.commit();
	return count;
}

static int countRows(Attachment& attachment, const char* table)
{
	TransactionOptions options;
	options.setIsolationLevel(TransactionIsolationLevel::READ_COMMITTED);

	Transaction transaction{attachment, options};
	std::string sql = "select count(*) from ";
	sql += table;
	Statement statement{attachment, transaction, sql};
	statement.execute(transaction);
	int count = statement.getInt32(0).value();
	transaction.commit();
	return count;
}

BOOST_AUTO_TEST_CASE(singleDatabasePrepareBasic)
{
	// Test prepare() on a regular single-database transaction
	// This bypasses the multi-database constructor
	Attachment attachment{
		CLIENT, getTempFile("Transaction2PC-singlePrepareBasic.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK(transaction.getState() == TransactionState::ACTIVE);

	transaction.prepare();
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	transaction.commit();
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(singleDatabasePrepareWithMessage)
{
	// Test prepare() with message on a regular single-database transaction
	Attachment attachment{
		CLIENT, getTempFile("Transaction2PC-singlePrepareMsg.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	transaction.prepare("test-transaction-id-123");
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	transaction.rollback();
	BOOST_CHECK(transaction.getState() == TransactionState::ROLLED_BACK);
}

BOOST_AUTO_TEST_CASE(multiDatabase2Attachments)
{
	Attachment attachment1{CLIENT, getTempFile("Transaction2PC-multiDatabase2Attachments-db1.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{CLIENT, getTempFile("Transaction2PC-multiDatabase2Attachments-db2.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};
	Transaction transaction{attachments};

	BOOST_CHECK_EQUAL(transaction.isValid(), true);
	BOOST_CHECK(transaction.getState() == TransactionState::ACTIVE);

	transaction.commit();
	BOOST_CHECK_EQUAL(transaction.isValid(), false);
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(multiDatabase3Attachments)
{
	Attachment attachment1{CLIENT, getTempFile("Transaction2PC-multiDatabase3Attachments-db1.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{CLIENT, getTempFile("Transaction2PC-multiDatabase3Attachments-db2.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	Attachment attachment3{CLIENT, getTempFile("Transaction2PC-multiDatabase3Attachments-db3.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment3Drop{attachment3};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2, attachment3};
	Transaction transaction{attachments, TransactionOptions().setIsolationLevel(TransactionIsolationLevel::SNAPSHOT)};

	BOOST_CHECK_EQUAL(transaction.isValid(), true);
	BOOST_CHECK(transaction.getState() == TransactionState::ACTIVE);

	transaction.commit();
	BOOST_CHECK_EQUAL(transaction.isValid(), false);
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(prepareCommit)
{
	Attachment attachment1{
		CLIENT, getTempFile("Transaction2PC-prepareCommit-db1.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{
		CLIENT, getTempFile("Transaction2PC-prepareCommit-db2.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};
	Transaction transaction{attachments};

	BOOST_CHECK(transaction.getState() == TransactionState::ACTIVE);

	transaction.prepare();
	BOOST_CHECK_EQUAL(transaction.isValid(), true);
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	transaction.commit();
	BOOST_CHECK_EQUAL(transaction.isValid(), false);
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(prepareRollback)
{
	Attachment attachment1{
		CLIENT, getTempFile("Transaction2PC-prepareRollback-db1.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{
		CLIENT, getTempFile("Transaction2PC-prepareRollback-db2.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};
	Transaction transaction{attachments};

	BOOST_CHECK(transaction.getState() == TransactionState::ACTIVE);

	transaction.prepare();
	BOOST_CHECK_EQUAL(transaction.isValid(), true);
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	transaction.rollback();
	BOOST_CHECK_EQUAL(transaction.isValid(), false);
	BOOST_CHECK(transaction.getState() == TransactionState::ROLLED_BACK);
}

BOOST_AUTO_TEST_CASE(prepareWithMessage)
{
	Attachment attachment1{
		CLIENT, getTempFile("Transaction2PC-prepareWithMessage-db1.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{
		CLIENT, getTempFile("Transaction2PC-prepareWithMessage-db2.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};
	Transaction transaction{attachments};

	transaction.prepare("test-transaction-123");
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	transaction.commit();
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(prepareWithBinaryMessage)
{
	Attachment attachment1{CLIENT, getTempFile("Transaction2PC-prepareWithBinaryMessage-db1.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{CLIENT, getTempFile("Transaction2PC-prepareWithBinaryMessage-db2.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};
	Transaction transaction{attachments};

	std::vector<std::uint8_t> message{0x01, 0x02, 0x03, 0x04};
	transaction.prepare(message);
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	transaction.commit();
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(commitWithoutPrepare)
{
	Attachment attachment1{CLIENT, getTempFile("Transaction2PC-commitWithoutPrepare-db1.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{CLIENT, getTempFile("Transaction2PC-commitWithoutPrepare-db2.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};
	Transaction transaction{attachments};

	BOOST_CHECK(transaction.getState() == TransactionState::ACTIVE);

	// Commit without prepare should work
	transaction.commit();
	BOOST_CHECK_EQUAL(transaction.isValid(), false);
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(statementAcrossMultipleDatabases)
{
	Attachment attachment1{CLIENT, getTempFile("Transaction2PC-statementAcrossMultipleDatabases-db1.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{CLIENT, getTempFile("Transaction2PC-statementAcrossMultipleDatabases-db2.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};

	// Create table in first database
	{  // scope
		Transaction setupTx{attachment1};
		Statement stmt1{attachment1, setupTx, "create table test_table (id integer)"};
		stmt1.execute(setupTx);
		setupTx.commit();
	}

	// Create table in second database
	{  // scope
		Transaction setupTx{attachment2};
		Statement stmt2{attachment2, setupTx, "create table test_table (id integer)"};
		stmt2.execute(setupTx);
		setupTx.commit();
	}

	Transaction transaction{attachments};

	// Insert data in first database
	{  // scope
		Statement stmt1{attachment1, transaction, "insert into test_table (id) values (1)"};
		stmt1.execute(transaction);
	}

	// Insert data in second database
	{  // scope
		Statement stmt2{attachment2, transaction, "insert into test_table (id) values (2)"};
		stmt2.execute(transaction);
	}

	// Prepare and commit
	BOOST_CHECK_EQUAL(countLimbo(attachment1), 0);
	BOOST_CHECK_EQUAL(countLimbo(attachment2), 0);

	transaction.prepare();
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	BOOST_CHECK_EQUAL(countLimbo(attachment1), 1);
	BOOST_CHECK_EQUAL(countLimbo(attachment2), 1);

	transaction.commit();
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);

	BOOST_CHECK_EQUAL(countLimbo(attachment1), 0);
	BOOST_CHECK_EQUAL(countLimbo(attachment2), 0);

	BOOST_CHECK_EQUAL(countRows(attachment1, "test_table"), 1);
	BOOST_CHECK_EQUAL(countRows(attachment2, "test_table"), 1);
}

BOOST_AUTO_TEST_CASE(singleDatabasePrepare)
{
	// Prepare() should also work on single-database transactions
	Attachment attachment{
		CLIENT, getTempFile("Transaction2PC-singleDatabasePrepare.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	std::vector<std::reference_wrapper<Attachment>> attachments{attachment};
	Transaction transaction{attachments};

	transaction.prepare();
	BOOST_CHECK(transaction.getState() == TransactionState::PREPARED);

	transaction.commit();
	BOOST_CHECK(transaction.getState() == TransactionState::COMMITTED);
}

BOOST_AUTO_TEST_CASE(prepareRollbackData)
{
	// Verify that rollback after prepare actually rolls back the data
	Attachment attachment1{
		CLIENT, getTempFile("Transaction2PC-prepareRollbackData-db1.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment1Drop{attachment1};

	Attachment attachment2{
		CLIENT, getTempFile("Transaction2PC-prepareRollbackData-db2.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachment2Drop{attachment2};

	// Create tables
	{  // scope
		Transaction setupTx1{attachment1};
		Statement stmt1{attachment1, setupTx1, "create table test_table (id integer)"};
		stmt1.execute(setupTx1);
		setupTx1.commit();
	}

	{  // scope
		Transaction setupTx2{attachment2};
		Statement stmt2{attachment2, setupTx2, "create table test_table (id integer)"};
		stmt2.execute(setupTx2);
		setupTx2.commit();
	}

	// Insert data and rollback after prepare
	{  // scope
		std::vector<std::reference_wrapper<Attachment>> attachments{attachment1, attachment2};
		Transaction transaction{attachments};

		{  // scope
			Statement stmt1{attachment1, transaction, "insert into test_table (id) values (1)"};
			stmt1.execute(transaction);
		}

		{  // scope
			Statement stmt2{attachment2, transaction, "insert into test_table (id) values (2)"};
			stmt2.execute(transaction);
		}

		BOOST_CHECK_EQUAL(countLimbo(attachment1), 0);
		BOOST_CHECK_EQUAL(countLimbo(attachment2), 0);

		transaction.prepare();

		BOOST_CHECK_EQUAL(countLimbo(attachment1), 1);
		BOOST_CHECK_EQUAL(countLimbo(attachment2), 1);

		transaction.rollback();
	}

	BOOST_CHECK_EQUAL(countRows(attachment1, "test_table"), 0);
	BOOST_CHECK_EQUAL(countRows(attachment2, "test_table"), 0);
}

BOOST_AUTO_TEST_SUITE_END()
