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
#include "fb-cpp/Attachment.h"
#include "fb-cpp/Batch.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <span>
#include <string>
#include <vector>


BOOST_AUTO_TEST_SUITE(BatchSuite)

BOOST_AUTO_TEST_CASE(constructorFromStatementAndExecute)
{
	const auto database = getTempFile("Batch-constructorFromStatementAndExecute.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null, name varchar(50))"};
		ddl.execute(transaction);
		transaction.commit();
	}

	// Insert using batch from Statement with addMessage().
	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into batch_test (id, name) values (?, ?)"};

		Batch batch{insert, transaction, BatchOptions().setRecordCounts(true)};

		insert.setInt32(0, 1);
		insert.setString(1, "Alice");
		batch.addMessage();

		insert.setInt32(0, 2);
		insert.setString(1, "Bob");
		batch.addMessage();

		insert.setInt32(0, 3);
		insert.setString(1, "Charlie");
		batch.addMessage();

		auto completionState = batch.execute();

		BOOST_CHECK_EQUAL(completionState.getSize(), 3U);
		BOOST_CHECK_EQUAL(completionState.getState(0), 1);
		BOOST_CHECK_EQUAL(completionState.getState(1), 1);
		BOOST_CHECK_EQUAL(completionState.getState(2), 1);
		BOOST_CHECK(!completionState.findError(0).has_value());

		transaction.commit();
	}

	// Verify inserted data.
	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select id, name from batch_test order by id"};

		BOOST_CHECK(select.execute(transaction));
		BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);
		BOOST_CHECK_EQUAL(select.getString(1).value(), "Alice");

		BOOST_CHECK(select.fetchNext());
		BOOST_CHECK_EQUAL(select.getInt32(0).value(), 2);
		BOOST_CHECK_EQUAL(select.getString(1).value(), "Bob");

		BOOST_CHECK(select.fetchNext());
		BOOST_CHECK_EQUAL(select.getInt32(0).value(), 3);
		BOOST_CHECK_EQUAL(select.getString(1).value(), "Charlie");

		BOOST_CHECK(!select.fetchNext());
	}
}

BOOST_AUTO_TEST_CASE(constructorFromAttachmentAndExecute)
{
	const auto database = getTempFile("Batch-constructorFromAttachmentAndExecute.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null, val integer)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	// Insert using batch from Attachment + SQL.
	{  // scope
		Transaction transaction{attachment};
		Batch batch{attachment, transaction, "insert into batch_test (id, val) values (?, ?)", 3,
			BatchOptions().setRecordCounts(true)};

		// Get metadata to build raw messages.
		auto metadata = batch.getMetadata();

		FbUniquePtr<fb::IStatus> tempStatus{CLIENT.newStatus()};
		impl::StatusWrapper tempWrapper{CLIENT, tempStatus.get()};
		const auto msgLength = metadata->getMessageLength(&tempWrapper);
		const auto idOffset = metadata->getOffset(&tempWrapper, 0);
		const auto idNullOffset = metadata->getNullOffset(&tempWrapper, 0);
		const auto valOffset = metadata->getOffset(&tempWrapper, 1);
		const auto valNullOffset = metadata->getNullOffset(&tempWrapper, 1);

		std::vector<std::byte> message(msgLength, std::byte{0});

		for (int i = 1; i <= 3; ++i)
		{
			*reinterpret_cast<std::int32_t*>(&message[idOffset]) = i;
			*reinterpret_cast<std::int16_t*>(&message[idNullOffset]) = FB_FALSE;
			*reinterpret_cast<std::int32_t*>(&message[valOffset]) = i * 100;
			*reinterpret_cast<std::int16_t*>(&message[valNullOffset]) = FB_FALSE;
			batch.add(1, message.data());
		}

		auto completionState = batch.execute();

		BOOST_CHECK_EQUAL(completionState.getSize(), 3U);
		BOOST_CHECK_EQUAL(completionState.getState(0), 1);
		BOOST_CHECK_EQUAL(completionState.getState(1), 1);
		BOOST_CHECK_EQUAL(completionState.getState(2), 1);

		transaction.commit();
	}

	// Verify.
	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select id, val from batch_test order by id"};

		BOOST_CHECK(select.execute(transaction));
		BOOST_CHECK_EQUAL(select.getInt32(0).value(), 1);
		BOOST_CHECK_EQUAL(select.getInt32(1).value(), 100);

		BOOST_CHECK(select.fetchNext());
		BOOST_CHECK_EQUAL(select.getInt32(0).value(), 2);
		BOOST_CHECK_EQUAL(select.getInt32(1).value(), 200);

		BOOST_CHECK(select.fetchNext());
		BOOST_CHECK_EQUAL(select.getInt32(0).value(), 3);
		BOOST_CHECK_EQUAL(select.getInt32(1).value(), 300);

		BOOST_CHECK(!select.fetchNext());
	}
}

BOOST_AUTO_TEST_CASE(moveConstructorTransfersOwnership)
{
	const auto database = getTempFile("Batch-moveConstructorTransfersOwnership.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into batch_test (id) values (?)"};

		Batch original{insert, transaction};
		BOOST_CHECK(original.isValid());

		Batch moved{std::move(original)};
		BOOST_CHECK(moved.isValid());
		BOOST_CHECK(!original.isValid());

		insert.setInt32(0, 42);
		moved.addMessage();

		auto completionState = moved.execute();
		BOOST_CHECK_EQUAL(completionState.getSize(), 1U);

		transaction.commit();
	}
}

BOOST_AUTO_TEST_CASE(executeReportsNoInfoWhenRecordCountsDisabled)
{
	const auto database = getTempFile("Batch-noInfo.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into batch_test (id) values (?)"};

		Batch batch{insert, transaction};

		insert.setInt32(0, 1);
		batch.addMessage();

		auto completionState = batch.execute();

		BOOST_CHECK_EQUAL(completionState.getSize(), 1U);
		BOOST_CHECK_EQUAL(completionState.getState(0), BatchCompletionState::SUCCESS_NO_INFO);

		transaction.commit();
	}
}

BOOST_AUTO_TEST_CASE(executeWithBadDataReportsExecuteFailed)
{
	const auto database = getTempFile("Batch-badData.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null primary key)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	// Insert duplicate keys to cause errors.
	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into batch_test (id) values (?)"};

		Batch batch{
			insert, transaction, BatchOptions().setMultiError(true).setRecordCounts(true).setDetailedErrors(10)};

		insert.setInt32(0, 1);
		batch.addMessage();

		insert.setInt32(0, 1);  // duplicate
		batch.addMessage();

		insert.setInt32(0, 2);
		batch.addMessage();

		auto completionState = batch.execute();

		BOOST_CHECK_EQUAL(completionState.getSize(), 3U);
		BOOST_CHECK_EQUAL(completionState.getState(0), 1);
		BOOST_CHECK_EQUAL(completionState.getState(1), BatchCompletionState::EXECUTE_FAILED);
		BOOST_CHECK_EQUAL(completionState.getState(2), 1);

		auto errorPos = completionState.findError(0);
		BOOST_REQUIRE(errorPos.has_value());
		BOOST_CHECK_EQUAL(errorPos.value(), 1U);

		// No more errors after position 1.
		BOOST_CHECK(!completionState.findError(errorPos.value() + 1).has_value());

		transaction.commit();
	}

	// Verify only valid rows were inserted.
	{  // scope
		Transaction transaction{attachment};
		Statement count{attachment, transaction, "select count(*) from batch_test"};

		BOOST_CHECK(count.execute(transaction));
		BOOST_CHECK_EQUAL(count.getInt32(0).value(), 2);
	}
}

BOOST_AUTO_TEST_CASE(cancelDiscardsMessages)
{
	const auto database = getTempFile("Batch-cancelDiscardsMessages.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into batch_test (id) values (?)"};

		Batch batch{insert, transaction};

		insert.setInt32(0, 1);
		batch.addMessage();

		batch.cancel();
		BOOST_CHECK(!batch.isValid());

		transaction.commit();
	}

	// Verify nothing was inserted.
	{  // scope
		Transaction transaction{attachment};
		Statement count{attachment, transaction, "select count(*) from batch_test"};

		BOOST_CHECK(count.execute(transaction));
		BOOST_CHECK_EQUAL(count.getInt32(0).value(), 0);
	}
}

BOOST_AUTO_TEST_CASE(blobWithIdEngine)
{
	const auto database = getTempFile("Batch-blobWithIdEngine.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null, data blob)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into batch_test (id, data) values (?, ?)"};

		Batch batch{insert, transaction, BatchOptions().setBlobPolicy(BlobPolicy::ID_ENGINE).setRecordCounts(true)};

		const std::string blobText = "Hello from batch blob!";
		const auto blobData = std::as_bytes(std::span{blobText});

		auto blobId = batch.addBlob(blobData);

		insert.setInt32(0, 1);
		insert.setBlobId(1, blobId);
		batch.addMessage();

		auto completionState = batch.execute();

		BOOST_CHECK_EQUAL(completionState.getSize(), 1U);
		BOOST_CHECK_EQUAL(completionState.getState(0), 1);

		transaction.commit();
	}

	// Verify blob content.
	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select data from batch_test where id = 1"};

		BOOST_CHECK(select.execute(transaction));

		const auto receivedBlobId = select.getBlobId(0);
		BOOST_REQUIRE(receivedBlobId.has_value());

		Blob reader{attachment, transaction, receivedBlobId.value(), BlobOptions().setType(BlobType::STREAM)};

		std::vector<std::byte> buffer(1024);
		const auto read = reader.read(buffer);
		std::string result(reinterpret_cast<const char*>(buffer.data()), read);

		BOOST_CHECK_EQUAL(result, "Hello from batch blob!");
	}
}

BOOST_AUTO_TEST_CASE(registerExistingBlob)
{
	const auto database = getTempFile("Batch-registerExistingBlob.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null, data blob)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};

		// Create a regular blob first.
		Blob writer{attachment, transaction, BlobOptions().setType(BlobType::STREAM)};
		const std::string blobText = "Registered blob data";
		writer.write(std::as_bytes(std::span{blobText}));
		writer.close();
		const auto existingBlobId = writer.getId();

		Statement insert{attachment, transaction, "insert into batch_test (id, data) values (?, ?)"};

		Batch batch{insert, transaction, BatchOptions().setBlobPolicy(BlobPolicy::ID_ENGINE).setRecordCounts(true)};

		auto batchBlobId = batch.registerBlob(existingBlobId);

		insert.setInt32(0, 1);
		insert.setBlobId(1, batchBlobId);
		batch.addMessage();

		auto completionState = batch.execute();

		BOOST_CHECK_EQUAL(completionState.getSize(), 1U);
		BOOST_CHECK_EQUAL(completionState.getState(0), 1);

		transaction.commit();
	}

	// Verify blob content.
	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select data from batch_test where id = 1"};

		BOOST_CHECK(select.execute(transaction));

		const auto receivedBlobId = select.getBlobId(0);
		BOOST_REQUIRE(receivedBlobId.has_value());

		Blob reader{attachment, transaction, receivedBlobId.value(), BlobOptions().setType(BlobType::STREAM)};

		std::vector<std::byte> buffer(1024);
		const auto read = reader.read(buffer);
		std::string result(reinterpret_cast<const char*>(buffer.data()), read);

		BOOST_CHECK_EQUAL(result, "Registered blob data");
	}
}

BOOST_AUTO_TEST_CASE(closeReleasesHandle)
{
	const auto database = getTempFile("Batch-closeReleasesHandle.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement ddl{attachment, transaction, "recreate table batch_test (id integer not null)"};
		ddl.execute(transaction);
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into batch_test (id) values (?)"};

		Batch batch{insert, transaction};
		BOOST_CHECK(batch.isValid());

		batch.close();
		BOOST_CHECK(!batch.isValid());
	}
}

BOOST_AUTO_TEST_SUITE_END()
