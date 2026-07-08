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
#include "fb-cpp/Attachment.h"
#include "fb-cpp/AttachmentPool.h"
#include "fb-cpp/Exception.h"
#include "fb-cpp/Transaction.h"
#include <chrono>
#include <string>

using namespace std::chrono_literals;


BOOST_AUTO_TEST_SUITE(AttachmentPoolSuite)

BOOST_AUTO_TEST_CASE(lifecycleAndCounts)
{
	const auto database = getTempFile("AttachmentPool-lifecycleAndCounts.fdb");
	Attachment setup{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase setupDrop{setup};

	{  // scope
		Transaction transaction{setup};
		BOOST_REQUIRE(setup.execute(transaction, "create table t (id integer not null primary key)"));
		transaction.commit();
	}

	AttachmentPoolOptions options;
	options.setAttachmentOptions(AttachmentOptions().setConnectionCharSet("UTF8")).setMinSize(2u).setMaxSize(4u);

	AttachmentPool pool{CLIENT, database, options};
	BOOST_CHECK_EQUAL(pool.size(), 2u);
	BOOST_CHECK_EQUAL(pool.availableCount(), 2u);
	BOOST_CHECK_EQUAL(pool.inUseCount(), 0u);

	{  // scope
		auto lease = pool.acquire();
		BOOST_CHECK(lease.isValid());
		BOOST_CHECK_EQUAL(pool.inUseCount(), 1u);
		BOOST_CHECK_EQUAL(pool.availableCount(), 1u);

		Transaction transaction{*lease};
		BOOST_REQUIRE(lease->execute(transaction, "insert into t (id) values (1)"));
		const auto count = lease->queryScalar<std::int32_t>(transaction, "select count(*) from t");
		BOOST_REQUIRE(count.has_value());
		BOOST_CHECK_EQUAL(*count, 1);
		transaction.commit();
	}

	BOOST_CHECK_EQUAL(pool.inUseCount(), 0u);
	BOOST_CHECK_EQUAL(pool.availableCount(), 2u);
}

BOOST_AUTO_TEST_CASE(acquireReusesUnderlyingConnection)
{
	const auto database = getTempFile("AttachmentPool-acquireReusesUnderlyingConnection.fdb");
	Attachment setup{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase setupDrop{setup};

	AttachmentPool pool{CLIENT, database, AttachmentPoolOptions().setMinSize(1u).setMaxSize(1u)};

	fb::IAttachment* firstHandle = nullptr;

	{  // scope
		auto lease = pool.acquire();
		firstHandle = lease->getHandle().get();
	}

	{  // scope
		auto lease = pool.acquire();
		BOOST_CHECK_EQUAL(lease->getHandle().get(), firstHandle);
	}

	BOOST_CHECK_EQUAL(pool.size(), 1u);
}

BOOST_AUTO_TEST_CASE(acquireThrowsWhenExhaustedAndUnblocksOnRelease)
{
	const auto database = getTempFile("AttachmentPool-acquireThrowsWhenExhaustedAndUnblocksOnRelease.fdb");
	Attachment setup{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase setupDrop{setup};

	AttachmentPool pool{
		CLIENT, database, AttachmentPoolOptions().setMinSize(0u).setMaxSize(1u).setAcquireTimeout(200ms)};

	auto lease1 = pool.acquire();
	BOOST_CHECK_EQUAL(pool.inUseCount(), 1u);

	BOOST_CHECK_THROW(pool.acquire(), FbCppException);
	BOOST_CHECK(!pool.tryAcquire().has_value());

	lease1.release();
	BOOST_CHECK_EQUAL(pool.inUseCount(), 0u);

	auto lease2 = pool.acquire();
	BOOST_CHECK(lease2.isValid());
}

BOOST_AUTO_TEST_CASE(leaseSemantics)
{
	const auto database = getTempFile("AttachmentPool-leaseSemantics.fdb");
	Attachment setup{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase setupDrop{setup};

	AttachmentPool pool{CLIENT, database, AttachmentPoolOptions().setMinSize(1u).setMaxSize(1u)};

	auto lease1 = pool.acquire();
	BOOST_CHECK(lease1.isValid());

	auto lease2 = std::move(lease1);
	BOOST_CHECK(!lease1.isValid());
	BOOST_CHECK(lease2.isValid());
	BOOST_CHECK_EQUAL(pool.inUseCount(), 1u);

	lease2.release();
	BOOST_CHECK(!lease2.isValid());
	BOOST_CHECK_EQUAL(pool.inUseCount(), 0u);

	pool.close();
	BOOST_CHECK_THROW(pool.acquire(), FbCppException);
}

BOOST_AUTO_TEST_CASE(sessionResetOnRelease)
{
	const auto database = getTempFile("AttachmentPool-sessionResetOnRelease.fdb");
	Attachment setup{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setForcedWrites(false)};
	FbDropDatabase setupDrop{setup};

	AttachmentPool pool{
		CLIENT, database, AttachmentPoolOptions().setMinSize(1u).setMaxSize(1u).setSessionResetOnRelease(true)};

	fb::IAttachment* firstHandle = nullptr;

	{  // scope
		auto lease = pool.acquire();
		firstHandle = lease->getHandle().get();

		Transaction transaction{*lease};
		lease->queryScalar<std::string>(
			transaction, "select rdb$set_context('USER_SESSION', 'k', 'v') from rdb$database");
		transaction.commit();
	}

	{  // scope
		auto lease = pool.acquire();
		BOOST_CHECK_EQUAL(lease->getHandle().get(), firstHandle);

		Transaction transaction{*lease};
		const auto value = lease->queryScalar<std::string>(
			transaction, "select rdb$get_context('USER_SESSION', 'k') from rdb$database");
		BOOST_CHECK(!value.has_value());
		transaction.commit();
	}
}

BOOST_AUTO_TEST_SUITE_END()
