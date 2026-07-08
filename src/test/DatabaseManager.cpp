/*
 * MIT License
 *
 * Copyright (c) 2026 Popa Adrian Marius
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
#include "fb-cpp/BackupManager.h"
#include "fb-cpp/DatabaseManager.h"
#include "fb-cpp/Exception.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <boost/test/unit_test.hpp>

namespace
{
	ServiceManagerOptions makeServiceManagerOptions()
	{
		auto options = ServiceManagerOptions{};

		if (const auto server = getServer())
			options.setServer(server.value());

		return options;
	}
}  // namespace

BOOST_AUTO_TEST_SUITE(DatabaseManagerSuite)

BOOST_AUTO_TEST_CASE(setReplicaModeNone)
{
	const auto databasePath = getTempFile("DatabaseManager-setReplicaModeNone.fdb", false);
	const auto databaseUri = getTempFile("DatabaseManager-setReplicaModeNone.fdb");
	const auto attachmentOptions = AttachmentOptions().setConnectionCharSet("UTF8");

	{  // scope
		Attachment attachment{
			CLIENT, databaseUri, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
		Transaction transaction{attachment};

		Statement queryContext{
			attachment, transaction, "select rdb$get_context('SYSTEM', 'REPLICA_MODE') from rdb$database"};
		BOOST_REQUIRE(queryContext.execute(transaction));
		BOOST_CHECK(!queryContext.getString(0).has_value());

		Statement queryMon{attachment, transaction, "select mon$replica_mode from mon$database"};
		BOOST_REQUIRE(queryMon.execute(transaction));
		BOOST_CHECK_EQUAL(queryMon.getInt32(0).value(), 0);
	}

	DatabaseManager manager{CLIENT, makeServiceManagerOptions()};
	manager.setProperties(DatabasePropertiesOptions().setDatabase(databasePath).setReplicaMode(ReplicaMode::READ_ONLY));

	{  // scope
		Attachment attachment{CLIENT, databaseUri, attachmentOptions};
		Transaction transaction{attachment};

		Statement queryContext{
			attachment, transaction, "select rdb$get_context('SYSTEM', 'REPLICA_MODE') from rdb$database"};
		BOOST_REQUIRE(queryContext.execute(transaction));
		BOOST_REQUIRE(queryContext.getString(0).has_value());
		BOOST_CHECK_EQUAL(queryContext.getString(0).value(), "READ-ONLY");

		Statement queryMon{attachment, transaction, "select mon$replica_mode from mon$database"};
		BOOST_REQUIRE(queryMon.execute(transaction));
		BOOST_CHECK_EQUAL(queryMon.getInt32(0).value(), 1);
	}

	manager.setProperties(DatabasePropertiesOptions().setDatabase(databasePath).setReplicaMode(ReplicaMode::NONE));

	{  // scope
		Attachment attachment{CLIENT, databaseUri, attachmentOptions};
		Transaction transaction{attachment};

		Statement queryContext{
			attachment, transaction, "select rdb$get_context('SYSTEM', 'REPLICA_MODE') from rdb$database"};
		BOOST_REQUIRE(queryContext.execute(transaction));
		BOOST_CHECK(!queryContext.getString(0).has_value());

		Statement queryMon{attachment, transaction, "select mon$replica_mode from mon$database"};
		BOOST_REQUIRE(queryMon.execute(transaction));
		BOOST_CHECK_EQUAL(queryMon.getInt32(0).value(), 0);
	}

	Attachment cleanup{CLIENT, databaseUri, attachmentOptions};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_CASE(restoreWithReplicaMode)
{
	const auto sourceDatabasePath = getTempFile("DatabaseManager-restoreReplica-source.fdb", false);
	const auto restoredDatabasePath = getTempFile("DatabaseManager-restoreReplica-restored.fdb", false);
	const auto backupFile = getTempFile("DatabaseManager-restoreReplica.fbk", false);
	const auto sourceDatabaseUri = getTempFile("DatabaseManager-restoreReplica-source.fdb");
	const auto restoredDatabaseUri = getTempFile("DatabaseManager-restoreReplica-restored.fdb");
	const auto attachmentOptions = AttachmentOptions().setConnectionCharSet("UTF8");

	{  // scope
		Attachment attachment{
			CLIENT, sourceDatabaseUri, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
		Transaction transaction{attachment};
		Statement create{attachment, transaction, "create table test(id integer)"};
		create.execute(transaction);
		transaction.commit();
	}

	BackupManager backupManager{CLIENT, makeServiceManagerOptions()};
	backupManager.backup(BackupOptions().setDatabase(sourceDatabasePath).setBackupFile(backupFile));
	backupManager.restore(RestoreOptions()
			.setDatabase(restoredDatabasePath)
			.setBackupFile(backupFile)
			.setReplicaMode(ReplicaMode::READ_WRITE));

	{  // scope
		Attachment restored{CLIENT, restoredDatabaseUri, attachmentOptions};
		FbDropDatabase restoredDrop{restored};
		Transaction transaction{restored};

		Statement queryContext{
			restored, transaction, "select rdb$get_context('SYSTEM', 'REPLICA_MODE') from rdb$database"};
		BOOST_REQUIRE(queryContext.execute(transaction));
		BOOST_REQUIRE(queryContext.getString(0).has_value());
		BOOST_CHECK_EQUAL(queryContext.getString(0).value(), "READ-WRITE");

		Statement queryMon{restored, transaction, "select mon$replica_mode from mon$database"};
		BOOST_REQUIRE(queryMon.execute(transaction));
		BOOST_CHECK_EQUAL(queryMon.getInt32(0).value(), 2);
	}

	Attachment cleanup{CLIENT, sourceDatabaseUri, attachmentOptions};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_CASE(databaseSweepAndValidate)
{
	const auto databasePath = getTempFile("DatabaseManager-sweepAndValidate.fdb", false);
	const auto databaseUri = getTempFile("DatabaseManager-sweepAndValidate.fdb");
	const auto attachmentOptions = AttachmentOptions().setConnectionCharSet("UTF8");

	{  // scope
		Attachment attachment{
			CLIENT, databaseUri, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
		Transaction transaction{attachment};

		Statement createTable{attachment, transaction, "create table test (id integer)"};
		BOOST_REQUIRE(createTable.execute(transaction));
		transaction.commit();
	}

	{  // scope
		Attachment attachment{CLIENT, databaseUri, attachmentOptions};
		Transaction transaction{attachment};

		Statement insertData{attachment, transaction, "insert into test (id) values (1)"};
		BOOST_REQUIRE(insertData.execute(transaction));
		transaction.commit();
	}

	DatabaseManager manager{CLIENT, makeServiceManagerOptions()};

	// 1. Run database sweep
	BOOST_CHECK_NO_THROW(manager.repair(DatabaseRepairOptions().setDatabase(databasePath).setSweep(true)));

	// 2. Run multi-threaded database sweep
	BOOST_CHECK_NO_THROW(
		manager.repair(DatabaseRepairOptions().setDatabase(databasePath).setSweep(true).setParallelWorkers(4)));

	// 3. Run database validation
	BOOST_CHECK_NO_THROW(
		manager.repair(DatabaseRepairOptions().setDatabase(databasePath).setValidate(true).setFull(true)));

	// 4. Run database upgrade (minor ODS upgrade)
	BOOST_CHECK_NO_THROW(manager.repair(DatabaseRepairOptions().setDatabase(databasePath).setUpgradeDb(true)));

	Attachment cleanup{CLIENT, databaseUri, attachmentOptions};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_CASE(databaseShutdownAndOnline)
{
	const auto databasePath = getTempFile("DatabaseManager-shutdownAndOnline.fdb", false);
	const auto databaseUri = getTempFile("DatabaseManager-shutdownAndOnline.fdb");
	const auto attachmentOptions = AttachmentOptions().setConnectionCharSet("UTF8");

	{  // scope
		Attachment attachment{
			CLIENT, databaseUri, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
		Transaction transaction{attachment};
		Statement createTable{attachment, transaction, "create table test (id integer)"};
		BOOST_REQUIRE(createTable.execute(transaction));
		transaction.commit();
	}

	DatabaseManager manager{CLIENT, makeServiceManagerOptions()};

	// Shutdown the database
	manager.setProperties(DatabasePropertiesOptions()
			.setDatabase(databasePath)
			.setShutdownMode(ShutdownMode::FULL)
			.setShutdownType(ShutdownType::FORCED)
			.setShutdownTimeout(0));

	// Attachment should fail when the database is shutdown
	BOOST_CHECK_THROW(Attachment(CLIENT, databaseUri, attachmentOptions), DatabaseException);

	// Bring the database online
	manager.setProperties(DatabasePropertiesOptions().setDatabase(databasePath).setOnline(true));

	// Attachment should succeed when online
	{  // scope
		Attachment attachment{CLIENT, databaseUri, attachmentOptions};
		Transaction transaction{attachment};
		Statement query{attachment, transaction, "select count(*) from test"};
		BOOST_REQUIRE(query.execute(transaction));
		BOOST_CHECK_EQUAL(query.getInt32(0).value(), 0);
	}

	Attachment cleanup{CLIENT, databaseUri, attachmentOptions};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_SUITE_END()
