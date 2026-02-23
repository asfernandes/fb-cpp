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

BOOST_AUTO_TEST_CASE(syntaxErrorExceptionProperties)
{
	const auto database = getTempFile("Exception-syntaxErrorExceptionProperties.fdb");

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
		// what() should contain a formatted error message
		std::string message = ex.what();
		BOOST_CHECK(!message.empty());

		// Error vector should start with isc_arg_gds and end with isc_arg_end
		const auto& errors = ex.getErrors();
		BOOST_REQUIRE(errors.size() >= 2);
		BOOST_CHECK_EQUAL(errors[0], isc_arg_gds);
		BOOST_CHECK_EQUAL(errors.back(), isc_arg_end);

		// getErrorCode() should return the first GDS code
		BOOST_CHECK_NE(ex.getErrorCode(), 0);
		BOOST_CHECK_EQUAL(ex.getErrorCode(), errors[1]);

		// SQL state, when present, should be a 5-character string
		if (!ex.getSqlState().empty())
			BOOST_CHECK_EQUAL(ex.getSqlState().size(), 5u);
	}
}

BOOST_AUTO_TEST_SUITE_END()
