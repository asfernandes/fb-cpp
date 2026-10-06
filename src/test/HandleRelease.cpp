/*
 * MIT License
 *
 * Copyright (c) 2026 Adriano dos Santos Fernandes
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
#include "fb-cpp/Blob.h"
#include "fb-cpp/ServiceManager.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"


BOOST_AUTO_TEST_SUITE(HandleReleaseSuite)

namespace
{
	// Holds two extra references to a Firebird interface around a call that ends its life.
	// released() returns how many references the call gave back and drops the extra ones.
	class ReleaseProbe final
	{
	public:
		// Takes the temporary reference returned by getHandle() methods and drops it before counting.
		template <typename T>
		explicit ReleaseProbe(FbRef<T> handle)
			: ptr{handle.get()}
		{
			handle.reset();

			ptr->addRef();
			before = ptr->release();
			ptr->addRef();
			ptr->addRef();
		}

	public:
		int released()
		{
			const int after = ptr->release();

			if (after > 0)
				ptr->release();

			return before + 1 - after;
		}

	private:
		fb::IReferenceCounted* ptr;
		int before = 0;
	};

	ServiceManagerOptions makeServiceManagerOptions()
	{
		auto options = ServiceManagerOptions{};

		if (const auto server = getServer())
			options.setServer(server.value());

		return options;
	}
}  // namespace

BOOST_AUTO_TEST_CASE(handlesReleasedOnce)
{
	const auto database = getTempFile("HandleRelease-handlesReleasedOnce.fdb");

	Attachment attachment{getClient(), database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};

	{  // scope
		Transaction transaction{attachment};
		ReleaseProbe probe{transaction.getHandle()};
		transaction.commit();
		BOOST_CHECK_EQUAL(probe.released(), 1);
	}

	{  // scope
		Transaction transaction{attachment};
		ReleaseProbe probe{transaction.getHandle()};
		transaction.rollback();
		BOOST_CHECK_EQUAL(probe.released(), 1);
	}

	{  // scope
		Transaction transaction{attachment};
		Statement statement{attachment, transaction, "select 1 from rdb$database"};

		statement.execute(transaction);
		ReleaseProbe reexecutedResultSet{statement.getResultSetHandle()};
		statement.execute(transaction);
		BOOST_CHECK_EQUAL(reexecutedResultSet.released(), 1);

		ReleaseProbe resultSet{statement.getResultSetHandle()};
		ReleaseProbe stmt{statement.getStatementHandle()};
		statement.free();
		BOOST_CHECK_EQUAL(resultSet.released(), 1);
		BOOST_CHECK_EQUAL(stmt.released(), 1);

		Blob closedBlob{attachment, transaction};
		ReleaseProbe closedBlobProbe{closedBlob.getHandle()};
		closedBlob.close();
		BOOST_CHECK_EQUAL(closedBlobProbe.released(), 1);

		Blob cancelledBlob{attachment, transaction};
		ReleaseProbe cancelledBlobProbe{cancelledBlob.getHandle()};
		cancelledBlob.cancel();
		BOOST_CHECK_EQUAL(cancelledBlobProbe.released(), 1);

		transaction.commit();
	}

	{  // scope
		Attachment second{getClient(), database};
		ReleaseProbe probe{second.getHandle()};
		second.disconnect();
		BOOST_CHECK_EQUAL(probe.released(), 1);
	}

	{  // scope
		ServiceManager manager{getClient(), makeServiceManagerOptions()};
		ReleaseProbe probe{manager.getHandle()};
		manager.disconnect();
		BOOST_CHECK_EQUAL(probe.released(), 1);
	}

	ReleaseProbe probe{attachment.getHandle()};
	attachment.dropDatabase();
	BOOST_CHECK_EQUAL(probe.released(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
