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

#ifndef FBCPP_ATTACHMENT_H
#define FBCPP_ATTACHMENT_H

#include "fb-api.h"
#include "RowSet.h"
#include "SmartPtrs.h"
#include "StatementOptions.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Client;
	class Statement;
	class Transaction;

	///
	/// Represents options used when creating an Attachment object.
	///
	class AttachmentOptions final
	{
	public:
		///
		/// Returns the character set which will be used for the connection.
		///
		const std::optional<std::string>& getConnectionCharSet() const
		{
			return connectionCharSet;
		}

		///
		/// Sets the character set which will be used for the connection.
		///
		AttachmentOptions& setConnectionCharSet(const std::string& value)
		{
			connectionCharSet = value;
			return *this;
		}

		///
		/// Returns the user name which will be used to connect to the database.
		///
		const std::optional<std::string>& getUserName() const
		{
			return userName;
		}

		///
		/// Sets the user name which will be used to connect to the database.
		///
		AttachmentOptions& setUserName(const std::string& value)
		{
			userName = value;
			return *this;
		}

		///
		/// Returns the password which will be used to connect to the database.
		///
		const std::optional<std::string>& getPassword() const
		{
			return password;
		}

		///
		/// Sets the password which will be used to connect to the database.
		///
		AttachmentOptions& setPassword(const std::string& value)
		{
			password = value;
			return *this;
		}

		///
		/// Returns the role which will be used to connect to the database.
		///
		const std::optional<std::string>& getRole() const
		{
			return role;
		}

		///
		/// Sets the role which will be used to connect to the database.
		///
		AttachmentOptions& setRole(const std::string& value)
		{
			role = value;
			return *this;
		}

		///
		/// Returns the SQL dialect which will be used to connect to the database.
		///
		const std::optional<std::uint32_t>& getSqlDialect() const
		{
			return sqlDialect;
		}

		///
		/// Sets the SQL dialect which will be used to connect to the database.
		///
		AttachmentOptions& setSqlDialect(std::uint32_t value)
		{
			sqlDialect = value;
			return *this;
		}

		///
		/// Returns the DPB (Database Parameter Block) which will be used to connect to the database.
		///
		const std::vector<std::uint8_t>& getDpb() const
		{
			return dpb;
		}

		///
		/// Sets the DPB (Database Parameter Block) which will be used to connect to the database.
		///
		AttachmentOptions& setDpb(const std::vector<std::uint8_t>& value)
		{
			dpb = value;
			return *this;
		}

		///
		/// Sets the DPB (Database Parameter Block) which will be used to connect to the database.
		///
		AttachmentOptions& setDpb(std::vector<std::uint8_t>&& value)
		{
			dpb = std::move(value);
			return *this;
		}

		///
		/// Returns whether the database should be created instead of connected to.
		///
		bool getCreateDatabase() const
		{
			return createDatabase;
		}

		///
		/// Sets whether the database should be created instead of connected to.
		///
		AttachmentOptions& setCreateDatabase(bool value)
		{
			createDatabase = value;
			return *this;
		}

		///
		/// Returns whether forced writes should be enabled when creating the database.
		///
		const std::optional<bool>& getForcedWrites() const
		{
			return forcedWrites;
		}

		///
		/// Sets whether forced writes should be enabled when creating the database.
		///
		AttachmentOptions& setForcedWrites(bool value)
		{
			forcedWrites = value;
			return *this;
		}

	private:
		std::optional<std::string> connectionCharSet;
		std::optional<std::string> userName;
		std::optional<std::string> password;
		std::optional<std::string> role;
		std::optional<std::uint32_t> sqlDialect;
		std::optional<bool> forcedWrites;
		std::vector<std::uint8_t> dpb;
		bool createDatabase = false;
	};

	///
	/// Represents a connection to a Firebird database.
	/// The Attachment must exist and remain valid while there are other objects using it, such as Transaction and
	/// Statement.
	///
	class Attachment final
	{
	public:
		///
		/// Constructs an Attachment object that connects to (or creates) the database specified by the URI
		/// using the specified Client object and options.
		///
		explicit Attachment(Client& client, const std::string& uri, const AttachmentOptions& options = {});

		///
		/// Move constructor.
		/// A moved Attachment object becomes invalid.
		///
		Attachment(Attachment&& o) noexcept
			: client{o.client},
			  handle{std::move(o.handle)}
		{
		}

		///
		/// @brief Transfers ownership of another Attachment into this one.
		///
		/// The old handle is released via `FbRef::operator=(FbRef&&)`.
		/// After the assignment, `this` is valid (with `o`'s handle) and `o` is invalid.
		///
		Attachment& operator=(Attachment&& o) noexcept
		{
			if (this != &o)
			{
				client = o.client;
				handle = std::move(o.handle);
			}

			return *this;
		}

		Attachment(const Attachment&) = delete;
		Attachment& operator=(const Attachment&) = delete;

		///
		/// Disconnects from the database.
		///
		~Attachment() noexcept
		{
			if (isValid())
			{
				try
				{
					disconnectOrDrop(false);
				}
				catch (...)
				{
					// swallow
				}
			}
		}

	public:
		///
		/// Returns whether the Attachment object is valid.
		///
		bool isValid() noexcept
		{
			return handle != nullptr;
		}

		///
		/// Returns the Client object reference used to create this Attachment object.
		///
		Client& getClient() noexcept
		{
			return *client;
		}

		///
		/// Returns the internal Firebird IAttachment handle.
		///
		FbRef<fb::IAttachment> getHandle() noexcept
		{
			return handle;
		}

		///
		/// Disconnects from the database.
		///
		void disconnect();

		///
		/// Drops the database.
		///
		void dropDatabase();

		///
		/// Prepares and executes an SQL statement using the supplied transaction.
		///
		bool execute(Transaction& transaction, std::string_view sql, const StatementOptions& options = {});

		///
		/// Prepares, binds parameters, and executes an SQL statement using the supplied transaction.
		///
		template <typename Params>
		bool execute(Transaction& transaction, std::string_view sql, const Params& params);

		///
		/// Prepares, binds parameters, and executes an SQL statement using the supplied transaction and statement
		/// options.
		///
		template <typename Params>
		bool execute(
			Transaction& transaction, std::string_view sql, const StatementOptions& options, const Params& params);

		///
		/// Prepares and executes a query using the supplied transaction and returns up to maxRows rows.
		///
		RowSet queryRowSet(
			Transaction& transaction, std::string_view sql, unsigned maxRows, const StatementOptions& options = {});

		///
		/// Prepares, binds parameters, and executes a query using the supplied transaction and returns up to maxRows
		/// rows.
		///
		template <typename Params>
		RowSet queryRowSet(Transaction& transaction, std::string_view sql, unsigned maxRows, const Params& params);

		///
		/// Prepares, binds parameters, and executes a query using the supplied transaction and statement options and
		/// returns up to maxRows rows.
		///
		template <typename Params>
		RowSet queryRowSet(Transaction& transaction, std::string_view sql, unsigned maxRows,
			const StatementOptions& options, const Params& params);

		///
		/// Prepares and executes a query using the supplied transaction and returns the first column of the first row.
		///
		template <typename T>
		std::optional<T> queryScalar(
			Transaction& transaction, std::string_view sql, const StatementOptions& options = {});

		///
		/// Prepares, binds parameters, and executes a query using the supplied transaction and returns the first column
		/// of the first row.
		///
		template <typename T, typename Params>
		std::optional<T> queryScalar(Transaction& transaction, std::string_view sql, const Params& params);

		///
		/// Prepares, binds parameters, and executes a query using the supplied transaction and statement options and
		/// returns the first column of the first row.
		///
		template <typename T, typename Params>
		std::optional<T> queryScalar(
			Transaction& transaction, std::string_view sql, const StatementOptions& options, const Params& params);

		///
		/// Prepares and executes a query using the supplied transaction and returns the first row mapped as T.
		///
		template <typename T>
		std::optional<T> queryFirstRowAs(
			Transaction& transaction, std::string_view sql, const StatementOptions& options = {});

		///
		/// Prepares, binds parameters, and executes a query using the supplied transaction and returns the first row
		/// mapped as T.
		///
		template <typename T, typename Params>
		std::optional<T> queryFirstRowAs(Transaction& transaction, std::string_view sql, const Params& params);

		///
		/// Prepares, binds parameters, and executes a query using the supplied transaction and statement options and
		/// returns the first row mapped as T.
		///
		template <typename T, typename Params>
		std::optional<T> queryFirstRowAs(
			Transaction& transaction, std::string_view sql, const StatementOptions& options, const Params& params);

	private:
		void disconnectOrDrop(bool drop);
		RowSet queryPreparedRowSet(Statement& statement, Transaction& transaction, unsigned maxRows);

	private:
		Client* client;
		FbRef<fb::IAttachment> handle;
	};

	template <typename T>
	std::optional<T> Attachment::queryScalar(
		Transaction& transaction, std::string_view sql, const StatementOptions& options)
	{
		auto rowSet = queryRowSet(transaction, sql, 1u, options);

		if (rowSet.getCount() == 0u)
			return std::nullopt;

		return rowSet.getRow(0).get<std::optional<T>>(0);
	}

	template <typename T>
	std::optional<T> Attachment::queryFirstRowAs(
		Transaction& transaction, std::string_view sql, const StatementOptions& options)
	{
		auto rowSet = queryRowSet(transaction, sql, 1u, options);

		if (rowSet.getCount() == 0u)
			return std::nullopt;

		return rowSet.getRow(0).get<T>();
	}
}  // namespace fbcpp

#include "Statement.h"

namespace fbcpp
{
	template <typename Params>
	bool Attachment::execute(Transaction& transaction, std::string_view sql, const Params& params)
	{
		return execute(transaction, sql, StatementOptions{}, params);
	}

	template <typename Params>
	bool Attachment::execute(
		Transaction& transaction, std::string_view sql, const StatementOptions& options, const Params& params)
	{
		Statement statement{*this, transaction, sql, options};
		statement.set(params);
		return statement.execute(transaction);
	}

	template <typename Params>
	RowSet Attachment::queryRowSet(
		Transaction& transaction, std::string_view sql, unsigned maxRows, const Params& params)
	{
		return queryRowSet(transaction, sql, maxRows, StatementOptions{}, params);
	}

	template <typename Params>
	RowSet Attachment::queryRowSet(Transaction& transaction, std::string_view sql, unsigned maxRows,
		const StatementOptions& options, const Params& params)
	{
		Statement statement{*this, transaction, sql, options};
		statement.set(params);
		return queryPreparedRowSet(statement, transaction, maxRows);
	}

	template <typename T, typename Params>
	std::optional<T> Attachment::queryScalar(Transaction& transaction, std::string_view sql, const Params& params)
	{
		auto rowSet = queryRowSet(transaction, sql, 1u, params);

		if (rowSet.getCount() == 0u)
			return std::nullopt;

		return rowSet.getRow(0).template get<std::optional<T>>(0);
	}

	template <typename T, typename Params>
	std::optional<T> Attachment::queryScalar(
		Transaction& transaction, std::string_view sql, const StatementOptions& options, const Params& params)
	{
		auto rowSet = queryRowSet(transaction, sql, 1u, options, params);

		if (rowSet.getCount() == 0u)
			return std::nullopt;

		return rowSet.getRow(0).template get<std::optional<T>>(0);
	}

	template <typename T, typename Params>
	std::optional<T> Attachment::queryFirstRowAs(Transaction& transaction, std::string_view sql, const Params& params)
	{
		auto rowSet = queryRowSet(transaction, sql, 1u, params);

		if (rowSet.getCount() == 0u)
			return std::nullopt;

		return rowSet.getRow(0).template get<T>();
	}

	template <typename T, typename Params>
	std::optional<T> Attachment::queryFirstRowAs(
		Transaction& transaction, std::string_view sql, const StatementOptions& options, const Params& params)
	{
		auto rowSet = queryRowSet(transaction, sql, 1u, options, params);

		if (rowSet.getCount() == 0u)
			return std::nullopt;

		return rowSet.getRow(0).template get<T>();
	}
}  // namespace fbcpp

#endif  // FBCPP_ATTACHMENT_H
