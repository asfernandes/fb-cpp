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
#include "fb-cpp/BackupManager.h"
#include "fb-cpp/Exception.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <ostream>
#include <string_view>
#include <vector>


namespace data = boost::unit_test::data;


namespace
{
	ServiceManagerOptions makeServiceManagerOptions()
	{
		auto options = ServiceManagerOptions{};

		if (const auto server = getServer())
			options.setServer(server.value());

		return options;
	}

	struct BackupRestoreVerboseCase
	{
		const char* suffix;
		bool backupVerbose;
		bool restoreVerbose;
	};

	std::ostream& operator<<(std::ostream& os, const BackupRestoreVerboseCase& testCase)
	{
		return os << testCase.suffix;
	}

	std::string normalizedFilename(const std::filesystem::path& path)
	{
#ifdef _WIN32
		auto str = path.filename().string();
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
		return str;
#else
		return path.filename().string();
#endif
	}

	static const std::initializer_list<BackupRestoreVerboseCase> BACKUP_RESTORE_VERBOSE_CASES{
		{"quiet-quiet", false, false},
		{"verbose-quiet", true, false},
		{"quiet-verbose", false, true},
		{"verbose-verbose", true, true},
	};
}  // namespace


BOOST_AUTO_TEST_SUITE(BackupManagerSuite)

BOOST_AUTO_TEST_CASE(backupOptionsManageBackupFiles)
{
	BackupOptions options;
	options.addBackupFile("first.fbk").addBackupFile("second.fbk");
	BOOST_REQUIRE_EQUAL(options.getBackupFiles().size(), 2u);
	BOOST_CHECK_EQUAL(options.getBackupFiles()[0].path, "first.fbk");
	BOOST_CHECK_EQUAL(options.getBackupFiles()[1].path, "second.fbk");

	options.setBackupFile("only.fbk");
	BOOST_REQUIRE_EQUAL(options.getBackupFiles().size(), 1u);
	BOOST_CHECK_EQUAL(options.getBackupFiles()[0].path, "only.fbk");
}

BOOST_AUTO_TEST_CASE(backupOptionsRejectLengthAfterItemWithoutLength)
{
	BackupOptions options;
	options.addBackupFile("first.fbk");
	BOOST_CHECK_THROW(options.addBackupFile("second.fbk", 2048), std::invalid_argument);

	options.setBackupFile("only.fbk");
	BOOST_CHECK_THROW(options.addBackupFile("second.fbk", 2048), std::invalid_argument);

	BackupOptions validOptions;
	validOptions.addBackupFile("first.fbk", 1024).addBackupFile("second.fbk", 2048);
	BOOST_REQUIRE_EQUAL(validOptions.getBackupFiles().size(), 2u);
}

BOOST_AUTO_TEST_CASE(restoreOptionsManageBackupFiles)
{
	RestoreOptions options;
	options.addBackupFile("first.fbk").addBackupFile("second.fbk");
	BOOST_REQUIRE_EQUAL(options.getBackupFiles().size(), 2u);
	BOOST_CHECK_EQUAL(options.getBackupFiles()[0], "first.fbk");
	BOOST_CHECK_EQUAL(options.getBackupFiles()[1], "second.fbk");

	options.setBackupFile("only.fbk");
	BOOST_REQUIRE_EQUAL(options.getBackupFiles().size(), 1u);
	BOOST_CHECK_EQUAL(options.getBackupFiles()[0], "only.fbk");
}

BOOST_AUTO_TEST_CASE(restoreOptionsRejectLengthAfterItemWithoutLength)
{
	RestoreOptions options;
	options.addDatabaseFile("first.fdb");
	BOOST_CHECK_THROW(options.addDatabaseFile("second.fdb", 2048), std::invalid_argument);

	options.setDatabase("only.fdb");
	BOOST_CHECK_THROW(options.addDatabaseFile("second.fdb", 2048), std::invalid_argument);

	RestoreOptions validOptions;
	validOptions.addDatabaseFile("first.fdb", 1024).addDatabaseFile("second.fdb", 2048);
	BOOST_REQUIRE_EQUAL(validOptions.getDatabaseFiles().size(), 2u);
}

BOOST_AUTO_TEST_CASE(serviceManagerDisconnectAndMove)
{
	ServiceManager manager1{CLIENT, makeServiceManagerOptions()};
	BOOST_CHECK(manager1.isValid());

	auto manager2 = std::move(manager1);
	BOOST_CHECK(manager2.isValid());
	BOOST_CHECK(!manager1.isValid());

	manager2.disconnect();
	BOOST_CHECK(!manager2.isValid());
}

BOOST_DATA_TEST_CASE(backupAndRestoreRoundTrip, data::make(BACKUP_RESTORE_VERBOSE_CASES), testCase)
{
	const auto attachmentOptions = AttachmentOptions().setConnectionCharSet("UTF8");
	const auto prefix = std::string("BackupManager-backupAndRestoreRoundTrip-") + testCase.suffix;
	const auto sourceDatabasePath = getTempFile(prefix + "-source.fdb", false);
	const auto restoredDatabasePath = getTempFile(prefix + "-restored.fdb", false);
	const auto backupFile = getTempFile(prefix + ".fbk", false);
	const auto sourceDatabaseUri = getTempFile(prefix + "-source.fdb");
	const auto restoredDatabaseUri = getTempFile(prefix + "-restored.fdb");

	{  // scope
		Attachment attachment{
			CLIENT, sourceDatabaseUri, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};

		Transaction transaction{attachment};

		Statement create{
			attachment, transaction, "create table test(id integer not null primary key, name varchar(20))"};
		create.execute(transaction);
		transaction.commitRetaining();

		Statement insert{attachment, transaction, "insert into test(id, name) values (1, 'backup')"};
		insert.execute(transaction);
		transaction.commit();
	}

	BackupManager manager{CLIENT, makeServiceManagerOptions()};
	std::vector<std::string> backupVerboseLines;
	auto backupOptions = BackupOptions().setDatabase(sourceDatabasePath).setBackupFile(backupFile);

	if (testCase.backupVerbose)
		backupOptions.setVerboseOutput([&](const std::string_view line) { backupVerboseLines.emplace_back(line); });

	manager.backup(backupOptions);
	BOOST_CHECK_EQUAL(!backupVerboseLines.empty(), testCase.backupVerbose);

	std::vector<std::string> restoreVerboseLines;
	auto restoreOptions = RestoreOptions().setDatabase(restoredDatabasePath).setBackupFile(backupFile);

	if (testCase.restoreVerbose)
		restoreOptions.setVerboseOutput([&](const std::string_view line) { restoreVerboseLines.emplace_back(line); });

	manager.restore(restoreOptions);
	BOOST_CHECK_EQUAL(!restoreVerboseLines.empty(), testCase.restoreVerbose);

	Attachment restored{CLIENT, restoredDatabaseUri, attachmentOptions};
	FbDropDatabase restoredDrop{restored};
	Transaction transaction{restored};
	Statement query{restored, transaction, "select id, name from test"};
	BOOST_REQUIRE(query.execute(transaction));
	BOOST_CHECK_EQUAL(query.getInt32(0).value(), 1);
	BOOST_CHECK_EQUAL(query.getString(1).value(), "backup");
	transaction.commit();

	Attachment cleanup{CLIENT, sourceDatabaseUri, attachmentOptions};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_CASE(restoreReplace)
{
	const auto sourceDatabasePath = getTempFile("BackupManager-restoreReplace-source.fdb", false);
	const auto restoredDatabasePath = getTempFile("BackupManager-restoreReplace-restored.fdb", false);
	const auto backupFile = getTempFile("BackupManager-restoreReplace.fbk", false);
	const auto sourceDatabaseUri = getTempFile("BackupManager-restoreReplace-source.fdb");
	const auto restoredDatabaseUri = getTempFile("BackupManager-restoreReplace-restored.fdb");
	const auto attachmentOptions = AttachmentOptions().setConnectionCharSet("UTF8");

	{  // scope
		Attachment attachment{
			CLIENT, sourceDatabaseUri, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};

		Transaction transaction{attachment};

		Statement create{attachment, transaction, "create table test(id integer not null primary key)"};
		create.execute(transaction);
		transaction.commitRetaining();

		Statement insert{attachment, transaction, "insert into test(id) values (7)"};
		insert.execute(transaction);
		transaction.commit();
	}

	BackupManager manager{CLIENT, makeServiceManagerOptions()};
	manager.backup(BackupOptions().setDatabase(sourceDatabasePath).setBackupFile(backupFile));
	manager.restore(RestoreOptions().setDatabase(restoredDatabasePath).setBackupFile(backupFile));
	manager.restore(RestoreOptions().setDatabase(restoredDatabasePath).setBackupFile(backupFile).setReplace(true));

	Attachment restored{CLIENT, restoredDatabaseUri, attachmentOptions};
	FbDropDatabase restoredDrop{restored};
	Transaction transaction{restored};
	Statement query{restored, transaction, "select id from test"};
	BOOST_REQUIRE(query.execute(transaction));
	BOOST_CHECK_EQUAL(query.getInt32(0).value(), 7);
	transaction.commit();

	Attachment cleanup{CLIENT, sourceDatabaseUri, attachmentOptions};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_CASE(multiFileDatabaseAndBackupRoundTrip)
{
	const auto sourceDatabasePath = getTempFile("BackupManager-multiFile-source.fdb", false);
	const auto sourceSecondaryPath = getTempFile("BackupManager-multiFile-source-2.fdb", false);
	const auto restoredDatabasePath = getTempFile("BackupManager-multiFile-restored.fdb", false);
	const auto restoredSecondaryPath = getTempFile("BackupManager-multiFile-restored-2.fdb", false);
	const auto backupFile1 = getTempFile("BackupManager-multiFile-1.fbk", false);
	const auto backupFile2 = getTempFile("BackupManager-multiFile-2.fbk", false);
	const auto sourceDatabaseUri = getTempFile("BackupManager-multiFile-source.fdb");
	const auto restoredDatabaseUri = getTempFile("BackupManager-multiFile-restored.fdb");
	const auto attachmentOptions = AttachmentOptions().setConnectionCharSet("UTF8");

	{  // scope
		Attachment attachment{
			CLIENT, sourceDatabaseUri, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};

		Transaction transaction{attachment};

		Statement create{
			attachment, transaction, "create table test(id integer not null primary key, name varchar(50))"};
		create.execute(transaction);
		transaction.commitRetaining();

		Statement addFile{
			attachment, transaction, "alter database add file '" + sourceSecondaryPath + "' starting at page 241"};
		addFile.execute(transaction);
		transaction.commitRetaining();

		for (int i = 1; i <= 200; ++i)
		{
			Statement insert{attachment, transaction, "insert into test(id, name) values (?, ?)"};
			insert.setInt32(0, i);
			insert.setString(1, "row-" + std::to_string(i) + std::string(40, 'x'));
			insert.execute(transaction);
		}
		transaction.commitRetaining();

		Statement queryFiles{attachment, transaction,
			"select count(*), min(trim(rdb$file_name)) from rdb$files where rdb$file_sequence = 1"};
		BOOST_REQUIRE(queryFiles.execute(transaction));
		BOOST_CHECK_EQUAL(queryFiles.getInt64(0).value(), 1);
		BOOST_CHECK_EQUAL(normalizedFilename(queryFiles.getString(1).value()), normalizedFilename(sourceSecondaryPath));
		transaction.commit();
	}

	BackupManager manager{CLIENT, makeServiceManagerOptions()};
	manager.backup(
		BackupOptions().setDatabase(sourceDatabasePath).addBackupFile(backupFile1, 2048).addBackupFile(backupFile2));

	manager.restore(RestoreOptions()
			.setDatabase(restoredDatabasePath, 200)
			.addDatabaseFile(restoredSecondaryPath)
			.addBackupFile(backupFile1)
			.addBackupFile(backupFile2));

	Attachment restored{CLIENT, restoredDatabaseUri, attachmentOptions};
	FbDropDatabase restoredDrop{restored};
	Transaction transaction{restored};
	Statement query{restored, transaction, "select count(*), min(id), max(id) from test"};
	BOOST_REQUIRE(query.execute(transaction));
	BOOST_CHECK_EQUAL(query.getInt64(0).value(), 200);
	BOOST_CHECK_EQUAL(query.getInt32(1).value(), 1);
	BOOST_CHECK_EQUAL(query.getInt32(2).value(), 200);

	Statement queryFiles{
		restored, transaction, "select count(*), min(trim(rdb$file_name)) from rdb$files where rdb$file_sequence = 1"};
	BOOST_REQUIRE(queryFiles.execute(transaction));
	BOOST_CHECK_EQUAL(queryFiles.getInt64(0).value(), 1);
	BOOST_CHECK_EQUAL(normalizedFilename(queryFiles.getString(1).value()), normalizedFilename(restoredSecondaryPath));
	transaction.commit();

	Attachment cleanup{CLIENT, sourceDatabaseUri, attachmentOptions};
	cleanup.dropDatabase();
}

BOOST_AUTO_TEST_SUITE_END()
