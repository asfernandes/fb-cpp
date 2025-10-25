/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to do so, subject to the
 * following conditions:
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
#include "Attachment.h"
#include "Blob.h"
#include "Statement.h"
#include "Transaction.h"
#include <array>
#include <limits>
#include <span>
#include <string>
#include <vector>


BOOST_AUTO_TEST_SUITE(BlobSuite)

BOOST_AUTO_TEST_CASE(readWriteMultiSegment)
{
	const auto database = getTempFile("Blob-readWriteMultiSegment.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	// DDL
	{  // scope
		Transaction transaction{attachment};
		Statement stmt{attachment, transaction, "recreate table blob_test (id integer, data blob)"};
		BOOST_CHECK(stmt.execute(transaction));
		transaction.commit();
	}

	const auto streamOptions = BlobOptions().setType(BlobType::STREAM);

	const auto multiSegmentSize =
		static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + std::size_t{1024};

	std::string multiSegmentText(multiSegmentSize, '\0');

	for (std::size_t i = 0; i < multiSegmentText.size(); ++i)
		multiSegmentText[i] = static_cast<char>('A' + (i % 26));

	BlobId blobId;

	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into blob_test (id, data) values (?, ?)"};

		Blob writer{attachment, transaction, streamOptions};
		const std::span<const char> textSpan{multiSegmentText.data(), multiSegmentText.size()};
		writer.write(std::as_bytes(textSpan));
		writer.write(std::span<const std::byte>{});  // writing empty buffer is allowed
		writer.close();

		blobId = writer.getId();

		insert.setInt32(0, 1);
		insert.setBlobId(1, blobId);
		BOOST_CHECK(insert.execute(transaction));
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select data from blob_test where id = ?"};
		select.setInt32(0, 1);

		BOOST_CHECK(select.execute(transaction));

		const auto receivedBlobId = select.getBlobId(0);
		BOOST_REQUIRE(receivedBlobId.has_value());

		Blob reader{attachment, transaction, receivedBlobId.value(), streamOptions};

		BOOST_CHECK_EQUAL(reader.getLength(), multiSegmentText.size());

		std::vector<std::byte> buffer(multiSegmentText.size());
		const auto read = reader.read(buffer);
		BOOST_CHECK_EQUAL(read, multiSegmentText.size());

		std::string reconstructed(reinterpret_cast<const char*>(buffer.data()), read);
		BOOST_CHECK_EQUAL(reconstructed, multiSegmentText);

		std::array<std::byte, 64> tail{};
		BOOST_CHECK_EQUAL(reader.read(tail), 0U);

		reader.seek(BlobSeekMode::FROM_BEGIN, 0);

		std::array<std::byte, 32> prefix{};
		const auto prefixRead = reader.read(prefix);
		BOOST_CHECK_EQUAL(prefixRead, prefix.size());
		std::string prefixText(reinterpret_cast<const char*>(prefix.data()), prefixRead);
		BOOST_CHECK_EQUAL(prefixText, multiSegmentText.substr(0, prefixRead));

		reader.close();
	}
}

BOOST_AUTO_TEST_CASE(createWriteRead)
{
	const auto database = getTempFile("Blob-createWriteRead.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	// DDL
	{  // scope
		Transaction transaction{attachment};
		Statement stmt{attachment, transaction, "recreate table blob_test (id integer, data blob)"};
		BOOST_CHECK(stmt.execute(transaction));
		transaction.commit();
	}

	const std::string text = "Firebird blob support!";
	const auto firstPartSize = text.size() / 2;

	const auto streamOptions = BlobOptions().setType(BlobType::STREAM);

	BlobId blobId;

	// Insert row with blob.
	{  // scope
		Transaction transaction{attachment};
		Statement insert{attachment, transaction, "insert into blob_test (id, data) values (?, ?)"};

		Blob blobWriter{attachment, transaction, streamOptions};
		const auto firstPart = std::span{text.data(), firstPartSize};
		const auto secondPart = std::span{text.data() + firstPartSize, text.size() - firstPartSize};
		blobWriter.writeSegment(std::as_bytes(firstPart));
		blobWriter.writeSegment(std::as_bytes(secondPart));
		blobWriter.close();

		blobId = blobWriter.getId();

		insert.setInt32(0, 1);
		insert.setBlobId(1, blobId);
		BOOST_CHECK(insert.execute(transaction));
		transaction.commit();

		BOOST_CHECK_EQUAL(blobWriter.isValid(), false);
		BOOST_CHECK(blobId.isEmpty() == false);
	}

	// Query blob identifier.
	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select data from blob_test where id = ?"};
		select.setInt32(0, 1);

		BOOST_CHECK(select.execute(transaction));

		const auto receivedBlobId = select.getBlobId(0);
		BOOST_REQUIRE(receivedBlobId.has_value());
		BOOST_TEST(receivedBlobId->isEmpty() == false);

		Blob blobReader{attachment, transaction, receivedBlobId.value(), streamOptions};

		BOOST_CHECK_EQUAL(blobReader.getLength(), text.size());

		std::string result;
		std::array<std::byte, 4> buffer{};

		for (;;)
		{
			const auto read = blobReader.readSegment(buffer);

			if (read == 0)
				break;

			result.append(reinterpret_cast<const char*>(buffer.data()), read);
		}

		BOOST_CHECK_EQUAL(result, text);

		const auto seekPos = blobReader.seek(BlobSeekMode::FROM_BEGIN, 9);
		BOOST_CHECK_EQUAL(seekPos, 9);

		std::vector<std::byte> tailBytes(text.size());
		const auto tailRead = blobReader.readSegment(tailBytes);
		std::string tail(reinterpret_cast<const char*>(tailBytes.data()), tailRead);

		BOOST_CHECK_EQUAL(tail, text.substr(9));

		BOOST_CHECK_EQUAL(blobReader.readSegment(buffer), 0U);

		blobReader.close();
		BOOST_CHECK_EQUAL(blobReader.isValid(), false);
	}
}

BOOST_AUTO_TEST_CASE(cancelDiscardsHandle)
{
	const auto database = getTempFile("Blob-cancelDiscardsHandle.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};
	Blob blob{attachment, transaction};
	blob.cancel();
	BOOST_CHECK_EQUAL(blob.isValid(), false);
}

BOOST_AUTO_TEST_SUITE_END()
