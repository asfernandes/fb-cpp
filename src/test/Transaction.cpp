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
#include "Transaction.h"
#include "Exception.h"
#include <exception>


BOOST_AUTO_TEST_SUITE(TransactionSuite)

BOOST_AUTO_TEST_CASE(constructorWithOptions)
{
	Attachment attachment{
		CLIENT, getTempFile("Transaction-constructorWithOptions.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction1{
		attachment, TransactionOptions().setIsolationLevel(TransactionIsolationLevel::READ_COMMITTED)};
	transaction1.commit();

	Transaction transaction2{attachment,
		TransactionOptions()
			.setIsolationLevel(TransactionIsolationLevel::READ_COMMITTED)
			.setReadCommittedMode(TransactionReadCommittedMode::RECORD_VERSION)
			.setAccessMode(TransactionAccessMode::READ_WRITE)
			.setAutoCommit(false)
			.setNoAutoUndo(true)
			.setWaitMode(TransactionWaitMode::WAIT)};
	transaction2.rollback();
}

BOOST_AUTO_TEST_CASE(constructorWithSetTransactionCmd)
{
	Attachment attachment{CLIENT, getTempFile("Transaction-constructorWithSetTransactionCmd.fdb"),
		AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction1{attachment, "set transaction isolation level read committed"};
	transaction1.commit();

	Transaction transaction2{attachment, "set transaction isolation level snapshot"};
	transaction2.rollback();
}

BOOST_AUTO_TEST_CASE(destructor)
{
	Attachment attachment{
		CLIENT, getTempFile("Transaction-destructor.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
}

BOOST_AUTO_TEST_CASE(commit)
{
	Attachment attachment{CLIENT, getTempFile("Transaction-commit.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	transaction.commit();

	BOOST_CHECK_EQUAL(transaction.isValid(), false);
}

BOOST_AUTO_TEST_CASE(commitRetaining)
{
	Attachment attachment{
		CLIENT, getTempFile("Transaction-commitRetaining.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	transaction.commitRetaining();
	BOOST_CHECK_EQUAL(transaction.isValid(), true);

	transaction.commitRetaining();
}

BOOST_AUTO_TEST_CASE(rollback)
{
	Attachment attachment{CLIENT, getTempFile("Transaction-rollback.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	transaction.rollback();

	BOOST_CHECK_EQUAL(transaction.isValid(), false);
}

BOOST_AUTO_TEST_CASE(rollbackRetaining)
{
	Attachment attachment{
		CLIENT, getTempFile("Transaction-rollbackRetaining.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	transaction.rollbackRetaining();
	BOOST_CHECK_EQUAL(transaction.isValid(), true);

	transaction.rollbackRetaining();
}

BOOST_AUTO_TEST_CASE(isNotValidAfterMove)
{
	Attachment attachment{
		CLIENT, getTempFile("Transaction-isNotValidAfterMove.fdb"), AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	BOOST_CHECK_EQUAL(transaction.isValid(), true);

	auto transaction2 = std::move(transaction);
	BOOST_CHECK_EQUAL(transaction2.isValid(), true);
	BOOST_CHECK_EQUAL(transaction.isValid(), false);

	transaction2.commit();
	BOOST_CHECK_EQUAL(transaction2.isValid(), false);
}

BOOST_AUTO_TEST_SUITE_END()
