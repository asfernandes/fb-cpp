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
#include "fb-cpp/Exception.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <string>


BOOST_AUTO_TEST_SUITE(DatabaseExceptionSuite)

BOOST_AUTO_TEST_CASE(syntaxErrorHasNonZeroErrorCode)
{
	const auto database = getTempFile("Exception-syntaxErrorHasNonZeroErrorCode.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	try
	{
		Statement stmt{attachment, transaction, "INVALID SQL STATEMENT !!!"};
		BOOST_FAIL("Expected DatabaseException was not thrown");
	}
	catch (const DatabaseException& ex)
	{
		BOOST_CHECK_NE(ex.getErrorCode(), 0);
	}
}

BOOST_AUTO_TEST_CASE(errorVectorContainsIscArgGds)
{
	const auto database = getTempFile("Exception-errorVectorContainsIscArgGds.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	try
	{
		Statement stmt{attachment, transaction, "INVALID SQL"};
		BOOST_FAIL("Expected DatabaseException was not thrown");
	}
	catch (const DatabaseException& ex)
	{
		const auto& errors = ex.getErrors();
		BOOST_REQUIRE(!errors.empty());
		BOOST_CHECK_EQUAL(errors[0], isc_arg_gds);
	}
}

BOOST_AUTO_TEST_CASE(errorVectorIsTerminatedByIscArgEnd)
{
	const auto database = getTempFile("Exception-errorVectorIsTerminatedByIscArgEnd.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	try
	{
		Statement stmt{attachment, transaction, "INVALID SQL"};
		BOOST_FAIL("Expected DatabaseException was not thrown");
	}
	catch (const DatabaseException& ex)
	{
		const auto& errors = ex.getErrors();
		BOOST_REQUIRE(!errors.empty());
		BOOST_CHECK_EQUAL(errors.back(), isc_arg_end);
	}
}

BOOST_AUTO_TEST_CASE(getErrorCodeReturnsFirstGdsCode)
{
	const auto database = getTempFile("Exception-getErrorCodeReturnsFirstGdsCode.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	try
	{
		Statement stmt{attachment, transaction, "INVALID SQL"};
		BOOST_FAIL("Expected DatabaseException was not thrown");
	}
	catch (const DatabaseException& ex)
	{
		const auto& errors = ex.getErrors();
		BOOST_REQUIRE(errors.size() >= 2);
		BOOST_CHECK_EQUAL(ex.getErrorCode(), errors[1]);
	}
}

BOOST_AUTO_TEST_CASE(sqlStateIsExtractedForSyntaxError)
{
	const auto database = getTempFile("Exception-sqlStateIsExtractedForSyntaxError.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	try
	{
		Statement stmt{attachment, transaction, "INVALID SQL"};
		BOOST_FAIL("Expected DatabaseException was not thrown");
	}
	catch (const DatabaseException& ex)
	{
		// Firebird 3.0+ includes SQL state in the status vector.
		BOOST_CHECK(!ex.getSqlState().empty());
	}
}

BOOST_AUTO_TEST_CASE(defaultConstructedHasEmptyErrorVector)
{
	DatabaseException ex{"test error message"};
	BOOST_CHECK(ex.getErrors().empty());
	BOOST_CHECK_EQUAL(ex.getErrorCode(), 0);
	BOOST_CHECK(ex.getSqlState().empty());
}

BOOST_AUTO_TEST_CASE(whatPreservesFormattedMessage)
{
	const auto database = getTempFile("Exception-whatPreservesFormattedMessage.fdb");

	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true)};
	FbDropDatabase attachmentDrop{attachment};

	Transaction transaction{attachment};

	try
	{
		Statement stmt{attachment, transaction, "INVALID SQL"};
		BOOST_FAIL("Expected DatabaseException was not thrown");
	}
	catch (const DatabaseException& ex)
	{
		std::string message = ex.what();
		BOOST_CHECK(!message.empty());
	}
}

BOOST_AUTO_TEST_SUITE_END()
