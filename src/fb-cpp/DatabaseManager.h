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

#ifndef FBCPP_DATABASE_MANAGER_H
#define FBCPP_DATABASE_MANAGER_H

#include "ServiceManager.h"
#include <cstdint>
#include <optional>
#include <string>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Shutdown mode for a Firebird database.
	///
	enum class ShutdownMode
	{
		///
		/// Normal online state.
		///
		NORMAL,

		///
		/// Multi-user shutdown.
		///
		MULTI,

		///
		/// Single-user shutdown.
		///
		SINGLE,

		///
		/// Full exclusive shutdown.
		///
		FULL
	};

	///
	/// Shutdown type for a Firebird database.
	///
	enum class ShutdownType
	{
		///
		/// Forced shutdown: disconnects all users immediately after the timeout.
		///
		FORCED,

		///
		/// Deny new transactions: waits for existing transactions to finish.
		///
		DENY_TRANSACTIONS,

		///
		/// Deny new attachments: waits for existing connections to finish.
		///
		DENY_ATTACHMENTS
	};

	///
	/// Represents options used to configure database properties through the service manager.
	///
	class DatabasePropertiesOptions final
	{
	public:
		///
		/// Returns the database path to be configured.
		///
		const std::string& getDatabase() const
		{
			return database;
		}

		///
		/// Sets the database path to be configured.
		///
		DatabasePropertiesOptions& setDatabase(const std::string& value)
		{
			database = value;
			return *this;
		}

		///
		/// Returns the replica mode.
		///
		const std::optional<ReplicaMode>& getReplicaMode() const
		{
			return replicaMode;
		}

		///
		/// Sets the replica mode.
		///
		DatabasePropertiesOptions& setReplicaMode(ReplicaMode value)
		{
			replicaMode = value;
			return *this;
		}

		///
		/// Returns the shutdown mode.
		///
		const std::optional<ShutdownMode>& getShutdownMode() const
		{
			return shutdownMode;
		}

		///
		/// Sets the shutdown mode.
		///
		DatabasePropertiesOptions& setShutdownMode(ShutdownMode value)
		{
			shutdownMode = value;
			return *this;
		}

		///
		/// Returns the shutdown type.
		///
		const std::optional<ShutdownType>& getShutdownType() const
		{
			return shutdownType;
		}

		///
		/// Sets the shutdown type.
		///
		DatabasePropertiesOptions& setShutdownType(ShutdownType value)
		{
			shutdownType = value;
			return *this;
		}

		///
		/// Returns the shutdown timeout in seconds.
		///
		const std::optional<int>& getShutdownTimeout() const
		{
			return shutdownTimeout;
		}

		///
		/// Sets the shutdown timeout in seconds.
		///
		DatabasePropertiesOptions& setShutdownTimeout(int value)
		{
			shutdownTimeout = value;
			return *this;
		}

		///
		/// Returns whether the database should be brought online.
		///
		const std::optional<bool>& getOnline() const
		{
			return online;
		}

		///
		/// Sets whether the database should be brought online.
		///
		DatabasePropertiesOptions& setOnline(bool value)
		{
			online = value;
			return *this;
		}

	private:
		std::string database;
		std::optional<ReplicaMode> replicaMode;
		std::optional<ShutdownMode> shutdownMode;
		std::optional<ShutdownType> shutdownType;
		std::optional<int> shutdownTimeout;
		std::optional<bool> online;
	};

	///
	/// Represents options used to run a database maintenance operation through the service manager.
	///
	class DatabaseRepairOptions final
	{
	public:
		///
		/// Returns the database path to be maintained.
		///
		const std::string& getDatabase() const
		{
			return database;
		}

		///
		/// Sets the database path to be maintained.
		///
		DatabaseRepairOptions& setDatabase(const std::string& value)
		{
			database = value;
			return *this;
		}

		///
		/// Returns the verbose output callback.
		///
		const ServiceManager::VerboseOutput& getVerboseOutput() const
		{
			return verboseOutput;
		}

		///
		/// Sets the verbose output callback.
		///
		DatabaseRepairOptions& setVerboseOutput(ServiceManager::VerboseOutput value)
		{
			verboseOutput = std::move(value);
			return *this;
		}

		///
		/// Returns the requested number of parallel workers.
		///
		const std::optional<std::uint32_t>& getParallelWorkers() const
		{
			return parallelWorkers;
		}

		///
		/// Sets the requested number of parallel workers.
		///
		DatabaseRepairOptions& setParallelWorkers(std::uint32_t value)
		{
			parallelWorkers = value;
			return *this;
		}

		///
		/// Returns whether database sweep is configured to be run.
		///
		bool getSweep() const
		{
			return sweep;
		}

		///
		/// Sets whether database sweep should be run.
		///
		DatabaseRepairOptions& setSweep(bool value)
		{
			sweep = value;
			return *this;
		}

		///
		/// Returns whether database validation is configured to be run.
		///
		bool getValidate() const
		{
			return validate;
		}

		///
		/// Sets whether database validation should be run.
		///
		DatabaseRepairOptions& setValidate(bool value)
		{
			validate = value;
			return *this;
		}

		///
		/// Returns whether database mending is configured to be run.
		///
		bool getMend() const
		{
			return mend;
		}

		///
		/// Sets whether database mending should be run.
		///
		DatabaseRepairOptions& setMend(bool value)
		{
			mend = value;
			return *this;
		}

		///
		/// Returns whether checksum verification is configured to be ignored.
		///
		bool getIgnoreChecksum() const
		{
			return ignoreChecksum;
		}

		///
		/// Sets whether checksum verification should be ignored.
		///
		DatabaseRepairOptions& setIgnoreChecksum(bool value)
		{
			ignoreChecksum = value;
			return *this;
		}

		///
		/// Returns whether killing database shadows is configured to be run.
		///
		bool getKillShadows() const
		{
			return killShadows;
		}

		///
		/// Sets whether killing database shadows should be run.
		///
		DatabaseRepairOptions& setKillShadows(bool value)
		{
			killShadows = value;
			return *this;
		}

		///
		/// Returns whether full validation is configured to be run.
		///
		bool getFull() const
		{
			return full;
		}

		///
		/// Sets whether full validation should be run.
		///
		DatabaseRepairOptions& setFull(bool value)
		{
			full = value;
			return *this;
		}

		///
		/// Returns whether checking only metadata/structure is configured to be run.
		///
		bool getCheckDb() const
		{
			return checkDb;
		}

		///
		/// Sets whether checking only metadata/structure should be run.
		///
		DatabaseRepairOptions& setCheckDb(bool value)
		{
			checkDb = value;
			return *this;
		}

		///
		/// Returns whether recreating ICU indexes is configured to be run.
		///
		bool getIcu() const
		{
			return icu;
		}

		///
		/// Sets whether recreating ICU indexes should be run.
		///
		DatabaseRepairOptions& setIcu(bool value)
		{
			icu = value;
			return *this;
		}

		///
		/// Returns whether database upgrade is configured to be run.
		///
		bool getUpgradeDb() const
		{
			return upgradeDb;
		}

		///
		/// Sets whether database upgrade should be run.
		///
		DatabaseRepairOptions& setUpgradeDb(bool value)
		{
			upgradeDb = value;
			return *this;
		}

	private:
		std::string database;
		ServiceManager::VerboseOutput verboseOutput;
		std::optional<std::uint32_t> parallelWorkers;
		bool sweep = false;
		bool validate = false;
		bool mend = false;
		bool ignoreChecksum = false;
		bool killShadows = false;
		bool full = false;
		bool checkDb = false;
		bool icu = false;
		bool upgradeDb = false;
	};

	///
	/// Executes configuration and maintenance operations through the Firebird service manager.
	///
	class DatabaseManager final : public ServiceManager
	{
	public:
		using ServiceManager::ServiceManager;

	public:
		///
		/// Configures database properties using the provided options.
		///
		void setProperties(const DatabasePropertiesOptions& options);

		///
		/// Runs a repair operation using the provided options.
		///
		void repair(const DatabaseRepairOptions& options);
	};
}  // namespace fbcpp


#endif  // FBCPP_DATABASE_MANAGER_H
