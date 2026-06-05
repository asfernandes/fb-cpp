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
#include <optional>
#include <string>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Represents options used to configure database properties through the service manager.
	///
	class DatabaseManagerOptions final
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
		DatabaseManagerOptions& setDatabase(const std::string& value)
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
		DatabaseManagerOptions& setReplicaMode(ReplicaMode value)
		{
			replicaMode = value;
			return *this;
		}

	private:
		std::string database;
		std::optional<ReplicaMode> replicaMode;
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
		void execute(const DatabaseManagerOptions& options);
	};
}  // namespace fbcpp


#endif  // FBCPP_DATABASE_MANAGER_H
