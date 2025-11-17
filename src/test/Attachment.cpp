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
#include "Attachment.h"
#include "Exception.h"
#include <exception>


BOOST_AUTO_TEST_SUITE(AttachmentSuite)

BOOST_AUTO_TEST_CASE(constructor)
{
	const auto database = getTempFile("Attachment-constructor.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	attachment1.disconnect();

	Attachment attachment2{CLIENT, database};
	attachment2.dropDatabase();
}

BOOST_AUTO_TEST_CASE(disconnect)
{
	const auto database = getTempFile("Attachment-disconnect.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	attachment1.disconnect();

	Attachment attachment2{CLIENT, database, AttachmentOptions().setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment2};
}

BOOST_AUTO_TEST_CASE(dropDatabase)
{
	const auto database = getTempFile("Attachment-dropDatabase.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	attachment1.dropDatabase();

	BOOST_CHECK_THROW(Attachment(CLIENT, database), DatabaseException);
}

BOOST_AUTO_TEST_CASE(isNotValidAfterMove)
{
	const auto database = getTempFile("Attachment-isNotValidAfterMove.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	BOOST_CHECK_EQUAL(attachment1.isValid(), true);

	auto attachment2 = std::move(attachment1);
	FbDropDatabase attachmentDrop{attachment2};
	BOOST_CHECK_EQUAL(attachment2.isValid(), true);
	BOOST_CHECK_EQUAL(attachment1.isValid(), false);
}

BOOST_AUTO_TEST_CASE(isNotValidAfterDisconnect)
{
	const auto database = getTempFile("Attachment-isNotValidAfterDisconnect.fdb");
	Attachment attachment1{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	BOOST_CHECK_EQUAL(attachment1.isValid(), true);

	attachment1.disconnect();
	BOOST_CHECK_EQUAL(attachment1.isValid(), false);

	Attachment attachment2{CLIENT, database};
	attachment2.dropDatabase();
}

BOOST_AUTO_TEST_CASE(isNotValidAfterDropDatabase)
{
	Attachment attachment1{
		CLIENT, getTempFile("Attachment-isNotValidAfterDropDatabase.fdb"), AttachmentOptions().setCreateDatabase(true)};
	BOOST_CHECK_EQUAL(attachment1.isValid(), true);

	attachment1.dropDatabase();
	BOOST_CHECK_EQUAL(attachment1.isValid(), false);
}

BOOST_AUTO_TEST_SUITE_END()
