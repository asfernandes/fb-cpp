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

#ifndef FBCPP_DATABASE_PROPERTY_MANAGER_H
#define FBCPP_DATABASE_PROPERTY_MANAGER_H

#include "ServiceManager.h"
#include <string>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Shutdown mode for a Firebird database (Firebird 2.0+).
	///
	enum class ShutdownMode
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
	/// Represents options used to shut down a Firebird database.
	///
	class ShutdownOptions final
	{
	public:
		///
		/// Returns the database path.
		///
		const std::string& getDatabase() const
		{
			return database;
		}

		///
		/// Sets the database path.
		///
		ShutdownOptions& setDatabase(const std::string& value)
		{
			database = value;
			return *this;
		}

		///
		/// Returns the shutdown mode.
		///
		ShutdownMode getMode() const
		{
			return mode;
		}

		///
		/// Sets the shutdown mode. Defaults to ShutdownMode::FORCED.
		///
		ShutdownOptions& setMode(ShutdownMode value)
		{
			mode = value;
			return *this;
		}

		///
		/// Returns the timeout in seconds.
		///
		int getTimeout() const
		{
			return timeout;
		}

		///
		/// Sets the timeout in seconds (0 = immediate). Defaults to 0.
		///
		ShutdownOptions& setTimeout(int value)
		{
			timeout = value;
			return *this;
		}

	private:
		std::string database;
		ShutdownMode mode = ShutdownMode::FORCED;
		int timeout = 0;
	};

	///
	/// Executes database property operations (shutdown, startup) through
	/// the Firebird service manager.
	///
	class DatabasePropertyManager final : public ServiceManager
	{
	public:
		using ServiceManager::ServiceManager;

	public:
		///
		/// Shuts down a database using the given options.
		///
		void shutdown(const ShutdownOptions& options);

		///
		/// Brings a database back online (online = normal read/write access).
		///
		void startup(const std::string& database);
	};
}  // namespace fbcpp


#endif  // FBCPP_DATABASE_PROPERTY_MANAGER_H
