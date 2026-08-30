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

#ifndef FBCPP_STATEMENT_H
#define FBCPP_STATEMENT_H

#include "config.h"
#include "fb-cpp-export.h"
#include "fb-api.h"
#include "types.h"
#include "Blob.h"
#include "Client.h"
#include "Row.h"
#include "StatementOptions.h"
#include "NumericConverter.h"
#include "CalendarConverter.h"
#include "Descriptor.h"
#include "impl/ParameterSetter.h"
#include "SmartPtrs.h"
#include "Exception.h"
#include "StructBinding.h"
#include "VariantTypeTraits.h"
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
#include <boost/decimal.hpp>
#endif

///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Attachment;
	class Transaction;

	///
	/// @brief Distinguishes the semantic category of the prepared SQL statement.
	///
	enum class StatementType : unsigned
	{
		///
		/// Server classified the statement as a `SELECT`.
		///
		SELECT = isc_info_sql_stmt_select,
		///
		/// Server classified the statement as an `INSERT`.
		///
		INSERT = isc_info_sql_stmt_insert,
		///
		/// Server classified the statement as an `UPDATE`.
		///
		UPDATE = isc_info_sql_stmt_update,
		///
		/// Server classified the statement as a `DELETE`.
		///
		DELETE = isc_info_sql_stmt_delete,
		///
		/// Statement performs data definition operations.
		///
		DDL = isc_info_sql_stmt_ddl,
		///
		/// Statement reads a blob segment - legacy feature.
		///
		GET_SEGMENT = isc_info_sql_stmt_get_segment,
		///
		/// Statement writes a blob segment - legacy feature.
		///
		PUT_SEGMENT = isc_info_sql_stmt_put_segment,
		///
		/// Statement executes a stored procedure.
		///
		EXEC_PROCEDURE = isc_info_sql_stmt_exec_procedure,
		///
		/// Statement starts a new transaction.
		///
		START_TRANSACTION = isc_info_sql_stmt_start_trans,
		///
		/// Statement commits a transaction.
		///
		COMMIT = isc_info_sql_stmt_commit,
		///
		/// Statement rolls back a transaction.
		///
		ROLLBACK = isc_info_sql_stmt_rollback,
		///
		/// Cursor-based `SELECT` that allows updates.
		///
		SELECT_FOR_UPDATE = isc_info_sql_stmt_select_for_upd,
		///
		/// Statement sets a generator (sequence) value.
		///
		SET_GENERATOR = isc_info_sql_stmt_set_generator,
		///
		/// Statement manages a savepoint.
		///
		SAVEPOINT = isc_info_sql_stmt_savepoint,
	};

	///
	/// Prepares, executes, and fetches SQL statements against a Firebird attachment.
	///
	class FB_CPP_EXPORT Statement final : private impl::ParameterSetter<Statement, false>
	{
		friend class RowSet;
		friend class impl::ParameterSetter<Statement, false>;

	public:
		///
		/// Prepares an SQL statement.
		/// `attachment` supplies the database connection.
		/// `transaction` is used for statement preparation.
		/// `sql` is the text to prepare.
		/// `options` provides fine-grained prepare controls.
		///
		explicit Statement(Attachment& attachment, Transaction& transaction, std::string_view sql,
			const StatementOptions& options = {});

		///
		/// @brief Transfers ownership of an existing prepared statement.
		///
		Statement(Statement&& o) noexcept;

		///
		/// @brief Transfers ownership of another prepared statement into this one.
		///
		/// The old handles are released via `FbRef::operator=(FbRef&&)`.
		/// After the assignment, `this` is valid (with `o`'s state) and `o` is invalid.
		///
		Statement& operator=(Statement&& o) noexcept;

		Statement(const Statement&) = delete;
		Statement& operator=(const Statement&) = delete;

		///
		/// @brief Releases resources; ignores failures to keep destructor noexcept.
		///
		~Statement() noexcept
		{
			if (isValid())
			{
				try
				{
					free();
				}
				catch (...)
				{
					// swallow
				}
			}
		}

	public:
		///
		/// @name Handle accessors
		/// @{
		/// @brief Reports whether the statement currently owns a prepared handle.
		///

		///
		/// Returns the Attachment object reference used to create this Statement.
		///
		Attachment& getAttachment() noexcept
		{
			return *attachment;
		}

		///
		/// Returns whether the Statement object is valid.
		///
		bool isValid() noexcept
		{
			return statementHandle != nullptr;
		}

		///
		/// Returns whether the statement is positioned on a current output row.
		///
		/// A current row is present after a successful `execute()` or fetch that produced
		/// output, until the row is consumed by `RowSet` or replaced by a later fetch.
		///
		bool hasCurrentRow() const noexcept
		{
			return currentRow;
		}

		///
		/// @brief Provides direct access to the underlying Firebird statement handle.
		/// @return Smart pointer to the low-level `fb::IStatement` interface.
		///
		FbRef<fb::IStatement> getStatementHandle() noexcept
		{
			return statementHandle;
		}

		///
		/// @brief Provides access to the underlying Firebird currently open result set handle, if any.
		/// @return Smart pointer to the active result set interface.
		///
		FbRef<fb::IResultSet> getResultSetHandle() noexcept
		{
			return resultSetHandle;
		}

		///
		/// @brief Returns the metadata describing prepared input parameters.
		///
		FbRef<fb::IMessageMetadata> getInputMetadata() noexcept
		{
			return inMetadata;
		}

		///
		/// @brief Provides direct access to the raw input message buffer.
		///
		std::vector<std::byte>& getInputMessage() noexcept
		{
			return inMessage;
		}

		///
		/// @brief Returns the metadata describing columns produced by the statement.
		///
		FbRef<fb::IMessageMetadata> getOutputMetadata() noexcept
		{
			return outMetadata;
		}

		///
		/// @brief Provides direct access to the raw output message buffer.
		///
		std::vector<std::byte>& getOutputMessage() noexcept
		{
			return outMessage;
		}

		///
		/// @brief Returns the type classification reported by the server.
		///
		StatementType getType() noexcept
		{
			return type;
		}

		///
		/// @}
		///

		///
		/// @name Descriptor accessors
		/// @{
		/// @brief Provides cached descriptors for each input parameter.
		///

		///
		/// @brief Provides cached descriptors for each input column.
		///
		const std::vector<Descriptor>& getInputDescriptors() noexcept
		{
			return inDescriptors;
		}

		///
		/// @brief Provides cached descriptors for each output column.
		///
		const std::vector<Descriptor>& getOutputDescriptors() noexcept
		{
			return outDescriptors;
		}

		///
		/// @}
		///

		///
		/// @brief Releases the prepared handle and any associated result set.
		///
		void free();

		///
		/// @brief Retrieves the textual legacy plan if the server produced one.
		///
		std::string getLegacyPlan();

		///
		/// @brief Retrieves the structured textual plan if the server produced one.
		///
		std::string getPlan();

		///
		/// @brief Executes a prepared statement using the supplied transaction.
		/// @param transaction Transaction that will own the execution context.
		/// @return `true` when execution yields a record.
		///
		bool execute(Transaction& transaction);

		///
		/// @name Cursor movement
		/// @{

		///
		/// @brief Fetches the next row in the current result set.
		///
		bool fetchNext();

		///
		/// @brief Fetches the previous row in the current result set.
		///
		bool fetchPrior();

		///
		/// @brief Positions the cursor on the first row.
		///
		bool fetchFirst();

		///
		/// @brief Positions the cursor on the last row.
		///
		bool fetchLast();

		///
		/// @brief Positions the cursor on the given absolute row number.
		///
		bool fetchAbsolute(unsigned position);

		///
		/// @brief Moves the cursor by the requested relative offset.
		///
		bool fetchRelative(int offset);

		///
		/// @}
		///

		///
		/// @name Parameter writing
		/// @{
		using impl::ParameterSetter<Statement, false>::clearParameters;
		using impl::ParameterSetter<Statement, false>::set;
		using impl::ParameterSetter<Statement, false>::setBlobId;
		using impl::ParameterSetter<Statement, false>::setBool;
		using impl::ParameterSetter<Statement, false>::setBytes;
		using impl::ParameterSetter<Statement, false>::setDate;
		using impl::ParameterSetter<Statement, false>::setDouble;
		using impl::ParameterSetter<Statement, false>::setFloat;
		using impl::ParameterSetter<Statement, false>::setInt16;
		using impl::ParameterSetter<Statement, false>::setInt32;
		using impl::ParameterSetter<Statement, false>::setInt64;
		using impl::ParameterSetter<Statement, false>::setNull;
		using impl::ParameterSetter<Statement, false>::setOpaqueDate;
		using impl::ParameterSetter<Statement, false>::setOpaqueDecFloat16;
		using impl::ParameterSetter<Statement, false>::setOpaqueDecFloat34;
		using impl::ParameterSetter<Statement, false>::setOpaqueInt128;
		using impl::ParameterSetter<Statement, false>::setOpaqueTime;
		using impl::ParameterSetter<Statement, false>::setOpaqueTimeTz;
		using impl::ParameterSetter<Statement, false>::setOpaqueTimestamp;
		using impl::ParameterSetter<Statement, false>::setOpaqueTimestampTz;
		using impl::ParameterSetter<Statement, false>::setScaledInt16;
		using impl::ParameterSetter<Statement, false>::setScaledInt32;
		using impl::ParameterSetter<Statement, false>::setScaledInt64;
		using impl::ParameterSetter<Statement, false>::setScaledOpaqueInt128;
		using impl::ParameterSetter<Statement, false>::setString;
		using impl::ParameterSetter<Statement, false>::setTime;
		using impl::ParameterSetter<Statement, false>::setTimeTz;
		using impl::ParameterSetter<Statement, false>::setTimestamp;
		using impl::ParameterSetter<Statement, false>::setTimestampTz;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		using impl::ParameterSetter<Statement, false>::setBoostDecFloat16;
		using impl::ParameterSetter<Statement, false>::setBoostDecFloat34;
		using impl::ParameterSetter<Statement, false>::setBoostInt128;
		using impl::ParameterSetter<Statement, false>::setScaledBoostInt128;
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		using impl::ParameterSetter<Statement, false>::setBoostDecimal128;
		using impl::ParameterSetter<Statement, false>::setBoostDecimal32;
		using impl::ParameterSetter<Statement, false>::setBoostDecimal64;
#endif

		///
		/// @}
		///

		///
		/// @name Result reading
		/// @{

		///
		/// @brief Reports whether the most recently fetched row has a null at the given column.
		///
		bool isNull(unsigned index)
		{
			assert(isValid());
			return outRow->isNull(index);
		}

		///
		/// @brief Reads a boolean column from the current row.
		///
		std::optional<bool> getBool(unsigned index)
		{
			assert(isValid());
			return outRow->getBool(index);
		}

		///
		/// @brief Reads a 16-bit signed integer column.
		///
		std::optional<std::int16_t> getInt16(unsigned index)
		{
			assert(isValid());
			return outRow->getInt16(index);
		}

		///
		/// @brief Reads a scaled 16-bit signed integer column.
		///
		std::optional<ScaledInt16> getScaledInt16(unsigned index)
		{
			assert(isValid());
			return outRow->getScaledInt16(index);
		}

		///
		/// @brief Reads a 32-bit signed integer column.
		///
		std::optional<std::int32_t> getInt32(unsigned index)
		{
			assert(isValid());
			return outRow->getInt32(index);
		}

		///
		/// @brief Reads a scaled 32-bit signed integer column.
		///
		std::optional<ScaledInt32> getScaledInt32(unsigned index)
		{
			assert(isValid());
			return outRow->getScaledInt32(index);
		}

		///
		/// @brief Reads a 64-bit signed integer column.
		///
		std::optional<std::int64_t> getInt64(unsigned index)
		{
			assert(isValid());
			return outRow->getInt64(index);
		}

		///
		/// @brief Reads a scaled 64-bit signed integer column.
		///
		std::optional<ScaledInt64> getScaledInt64(unsigned index)
		{
			assert(isValid());
			return outRow->getScaledInt64(index);
		}

		///
		/// @brief Reads a Firebird scaled 128-bit integer column.
		///
		std::optional<ScaledOpaqueInt128> getScaledOpaqueInt128(unsigned index)
		{
			assert(isValid());
			return outRow->getScaledOpaqueInt128(index);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost 128-bit integer column.
		///
		std::optional<BoostInt128> getBoostInt128(unsigned index)
		{
			assert(isValid());
			return outRow->getBoostInt128(index);
		}

		///
		/// @brief Reads a scaled Boost 128-bit integer column.
		///
		std::optional<ScaledBoostInt128> getScaledBoostInt128(unsigned index)
		{
			assert(isValid());
			return outRow->getScaledBoostInt128(index);
		}
#endif

		///
		/// @brief Reads a single precision floating-point column.
		///
		std::optional<float> getFloat(unsigned index)
		{
			assert(isValid());
			return outRow->getFloat(index);
		}

		///
		/// @brief Reads a double precision floating-point column.
		///
		std::optional<double> getDouble(unsigned index)
		{
			assert(isValid());
			return outRow->getDouble(index);
		}

		///
		/// @brief Reads a Firebird 16-digit decimal floating-point column.
		///
		std::optional<OpaqueDecFloat16> getOpaqueDecFloat16(unsigned index)
		{
			assert(isValid());
			return outRow->getOpaqueDecFloat16(index);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost-based 16-digit decimal floating-point column.
		///
		std::optional<BoostDecFloat16> getBoostDecFloat16(unsigned index)
		{
			assert(isValid());
			return outRow->getBoostDecFloat16(index);
		}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		///
		/// @brief Reads a Boost.Decimal 7-digit decimal floating-point column.
		///
		std::optional<BoostDecimal32> getBoostDecimal32(unsigned index)
		{
			assert(isValid());
			return outRow->getBoostDecimal32(index);
		}

		///
		/// @brief Reads a Boost.Decimal 16-digit decimal floating-point column.
		///
		std::optional<BoostDecimal64> getBoostDecimal64(unsigned index)
		{
			assert(isValid());
			return outRow->getBoostDecimal64(index);
		}
#endif

		///
		/// @brief Reads a Firebird 34-digit decimal floating-point column.
		///
		std::optional<OpaqueDecFloat34> getOpaqueDecFloat34(unsigned index)
		{
			assert(isValid());
			return outRow->getOpaqueDecFloat34(index);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost-based 34-digit decimal floating-point column.
		///
		std::optional<BoostDecFloat34> getBoostDecFloat34(unsigned index)
		{
			assert(isValid());
			return outRow->getBoostDecFloat34(index);
		}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		///
		/// @brief Reads a Boost.Decimal 34-digit decimal floating-point column.
		///
		std::optional<BoostDecimal128> getBoostDecimal128(unsigned index)
		{
			assert(isValid());
			return outRow->getBoostDecimal128(index);
		}
#endif

		///
		/// @brief Reads a date column.
		///
		std::optional<Date> getDate(unsigned index)
		{
			assert(isValid());
			return outRow->getDate(index);
		}

		///
		/// @brief Reads a raw date column in Firebird's representation.
		///
		std::optional<OpaqueDate> getOpaqueDate(unsigned index)
		{
			assert(isValid());
			return outRow->getOpaqueDate(index);
		}

		///
		/// @brief Reads a time-of-day column without timezone.
		///
		std::optional<Time> getTime(unsigned index)
		{
			assert(isValid());
			return outRow->getTime(index);
		}

		///
		/// @brief Reads a raw time-of-day column in Firebird's representation.
		///
		std::optional<OpaqueTime> getOpaqueTime(unsigned index)
		{
			assert(isValid());
			return outRow->getOpaqueTime(index);
		}

		///
		/// @brief Reads a timestamp column without timezone.
		///
		std::optional<Timestamp> getTimestamp(unsigned index)
		{
			assert(isValid());
			return outRow->getTimestamp(index);
		}

		///
		/// @brief Reads a raw timestamp column in Firebird's representation.
		///
		std::optional<OpaqueTimestamp> getOpaqueTimestamp(unsigned index)
		{
			assert(isValid());
			return outRow->getOpaqueTimestamp(index);
		}

		///
		/// @brief Reads a time-of-day column with timezone.
		///
		std::optional<TimeTz> getTimeTz(unsigned index)
		{
			assert(isValid());
			return outRow->getTimeTz(index);
		}

		///
		/// @brief Reads a raw time-of-day column with timezone in Firebird's representation.
		///
		std::optional<OpaqueTimeTz> getOpaqueTimeTz(unsigned index)
		{
			assert(isValid());
			return outRow->getOpaqueTimeTz(index);
		}

		///
		/// @brief Reads a timestamp-with-time-zone column.
		///
		std::optional<TimestampTz> getTimestampTz(unsigned index)
		{
			assert(isValid());
			return outRow->getTimestampTz(index);
		}

		///
		/// @brief Reads a raw timestamp-with-time-zone column in Firebird's representation.
		///
		std::optional<OpaqueTimestampTz> getOpaqueTimestampTz(unsigned index)
		{
			assert(isValid());
			return outRow->getOpaqueTimestampTz(index);
		}

		///
		/// @brief Reads a blob identifier column.
		///
		std::optional<BlobId> getBlobId(unsigned index)
		{
			assert(isValid());
			return outRow->getBlobId(index);
		}

		///
		/// @brief Reads a textual column, applying number-to-string conversions when needed.
		///
		std::optional<std::string> getString(unsigned index)
		{
			assert(isValid());
			return outRow->getString(index);
		}

		///
		/// @brief Reads a text or varying column as its raw bytes.
		///
		std::optional<std::vector<std::byte>> getBytes(unsigned index)
		{
			assert(isValid());
			return outRow->getBytes(index);
		}

		///
		/// @}
		///

		///
		/// @brief Retrieves a column using the most appropriate typed accessor specialization.
		///
		template <typename T>
		T get(unsigned index)
		{
			assert(isValid());
			return outRow->get<T>(index);
		}

		///
		/// @brief Retrieves all output columns into a user-defined aggregate struct.
		/// @tparam T An aggregate type whose fields match the output column count and types.
		/// @return The populated struct with values from the current row.
		/// @throws FbCppException if field count mismatches output column count.
		/// @throws FbCppException if a NULL value is encountered for a non-optional field.
		///
		template <Aggregate T>
		T get()
		{
			assert(isValid());
			return outRow->get<T>();
		}

		///
		/// @brief Retrieves all output columns into a tuple-like type.
		/// @tparam T A tuple-like type (std::tuple, std::pair) whose elements match the output column count and types.
		/// @return The populated tuple with values from the current row.
		/// @throws FbCppException if element count mismatches output column count.
		/// @throws FbCppException if a NULL value is encountered for a non-optional element.
		///
		template <TupleLike T>
		T get()
		{
			assert(isValid());
			return outRow->get<T>();
		}

		///
		/// @brief Retrieves a column value as a user-defined variant type.
		/// @tparam V A std::variant type with possible C++ types. Use std::monostate for NULL.
		/// @param index Zero-based column index.
		/// @return The variant with column value, or std::monostate if NULL.
		/// @throws FbCppException if NULL but variant lacks std::monostate.
		/// @throws FbCppException if SQL type cannot convert to any alternative.
		///
		template <VariantLike V>
		V get(unsigned index)
		{
			assert(isValid());
			return outRow->get<V>(index);
		}

	private:
		const std::vector<Descriptor>& getSetterDescriptors() const noexcept
		{
			return inDescriptors;
		}

		std::vector<std::byte>& getSetterMessage() noexcept
		{
			return inMessage;
		}

		Client& getSetterClient() noexcept
		{
			return getClient();
		}

		void validateSetter() const noexcept
		{
			assert(statementHandle != nullptr);
		}

		Client& getClient() noexcept;

		void clearCurrentRow() noexcept
		{
			currentRow = false;
		}

	private:
		Attachment* attachment;
		impl::StatusWrapper statusWrapper;
		impl::CalendarConverter calendarConverter;
		impl::NumericConverter numericConverter;
		FbRef<fb::IStatement> statementHandle;
		FbRef<fb::IResultSet> resultSetHandle;
		FbRef<fb::IMessageMetadata> inMetadata;
		std::vector<Descriptor> inDescriptors;
		std::vector<std::byte> inMessage;
		FbRef<fb::IMessageMetadata> outMetadata;
		std::vector<Descriptor> outDescriptors;
		std::vector<std::byte> outMessage;
		std::unique_ptr<Row> outRow;
		StatementType type;
		unsigned cursorFlags = 0;
		bool currentRow = false;
	};

	///
	/// @name Convenience template specializations
	/// @{
	///

	template <>
	inline std::optional<bool> Statement::get<std::optional<bool>>(unsigned index)
	{
		return getBool(index);
	}

	template <>
	inline std::optional<BlobId> Statement::get<std::optional<BlobId>>(unsigned index)
	{
		return getBlobId(index);
	}

	template <>
	inline std::optional<std::int16_t> Statement::get<std::optional<std::int16_t>>(unsigned index)
	{
		return getInt16(index);
	}

	template <>
	inline std::optional<ScaledInt16> Statement::get<std::optional<ScaledInt16>>(unsigned index)
	{
		return getScaledInt16(index);
	}

	template <>
	inline std::optional<std::int32_t> Statement::get<std::optional<std::int32_t>>(unsigned index)
	{
		return getInt32(index);
	}

	template <>
	inline std::optional<ScaledInt32> Statement::get<std::optional<ScaledInt32>>(unsigned index)
	{
		return getScaledInt32(index);
	}

	template <>
	inline std::optional<std::int64_t> Statement::get<std::optional<std::int64_t>>(unsigned index)
	{
		return getInt64(index);
	}

	template <>
	inline std::optional<ScaledInt64> Statement::get<std::optional<ScaledInt64>>(unsigned index)
	{
		return getScaledInt64(index);
	}

	template <>
	inline std::optional<ScaledOpaqueInt128> Statement::get<std::optional<ScaledOpaqueInt128>>(unsigned index)
	{
		return getScaledOpaqueInt128(index);
	}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	inline std::optional<BoostInt128> Statement::get<std::optional<BoostInt128>>(unsigned index)
	{
		return getBoostInt128(index);
	}

	template <>
	inline std::optional<ScaledBoostInt128> Statement::get<std::optional<ScaledBoostInt128>>(unsigned index)
	{
		return getScaledBoostInt128(index);
	}
#endif

	template <>
	inline std::optional<float> Statement::get<std::optional<float>>(unsigned index)
	{
		return getFloat(index);
	}

	template <>
	inline std::optional<double> Statement::get<std::optional<double>>(unsigned index)
	{
		return getDouble(index);
	}

	template <>
	inline std::optional<OpaqueDecFloat16> Statement::get<std::optional<OpaqueDecFloat16>>(unsigned index)
	{
		return getOpaqueDecFloat16(index);
	}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	inline std::optional<BoostDecFloat16> Statement::get<std::optional<BoostDecFloat16>>(unsigned index)
	{
		return getBoostDecFloat16(index);
	}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
	template <>
	inline std::optional<BoostDecimal32> Statement::get<std::optional<BoostDecimal32>>(unsigned index)
	{
		return getBoostDecimal32(index);
	}

	template <>
	inline std::optional<BoostDecimal64> Statement::get<std::optional<BoostDecimal64>>(unsigned index)
	{
		return getBoostDecimal64(index);
	}
#endif

	template <>
	inline std::optional<OpaqueDecFloat34> Statement::get<std::optional<OpaqueDecFloat34>>(unsigned index)
	{
		return getOpaqueDecFloat34(index);
	}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	inline std::optional<BoostDecFloat34> Statement::get<std::optional<BoostDecFloat34>>(unsigned index)
	{
		return getBoostDecFloat34(index);
	}
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
	template <>
	inline std::optional<BoostDecimal128> Statement::get<std::optional<BoostDecimal128>>(unsigned index)
	{
		return getBoostDecimal128(index);
	}
#endif

	template <>
	inline std::optional<Date> Statement::get<std::optional<Date>>(unsigned index)
	{
		return getDate(index);
	}

	template <>
	inline std::optional<OpaqueDate> Statement::get<std::optional<OpaqueDate>>(unsigned index)
	{
		return getOpaqueDate(index);
	}

	template <>
	inline std::optional<Time> Statement::get<std::optional<Time>>(unsigned index)
	{
		return getTime(index);
	}

	template <>
	inline std::optional<OpaqueTime> Statement::get<std::optional<OpaqueTime>>(unsigned index)
	{
		return getOpaqueTime(index);
	}

	template <>
	inline std::optional<OpaqueTimestamp> Statement::get<std::optional<OpaqueTimestamp>>(unsigned index)
	{
		return getOpaqueTimestamp(index);
	}

	template <>
	inline std::optional<Timestamp> Statement::get<std::optional<Timestamp>>(unsigned index)
	{
		return getTimestamp(index);
	}

	template <>
	inline std::optional<TimeTz> Statement::get<std::optional<TimeTz>>(unsigned index)
	{
		return getTimeTz(index);
	}

	template <>
	inline std::optional<OpaqueTimeTz> Statement::get<std::optional<OpaqueTimeTz>>(unsigned index)
	{
		return getOpaqueTimeTz(index);
	}

	template <>
	inline std::optional<TimestampTz> Statement::get<std::optional<TimestampTz>>(unsigned index)
	{
		return getTimestampTz(index);
	}

	template <>
	inline std::optional<OpaqueTimestampTz> Statement::get<std::optional<OpaqueTimestampTz>>(unsigned index)
	{
		return getOpaqueTimestampTz(index);
	}

	template <>
	inline std::optional<std::string> Statement::get<std::optional<std::string>>(unsigned index)
	{
		return getString(index);
	}

	template <>
	inline std::optional<std::vector<std::byte>> Statement::get<std::optional<std::vector<std::byte>>>(unsigned index)
	{
		return getBytes(index);
	}

	///
	/// @}
	///
}  // namespace fbcpp


#endif  // FBCPP_STATEMENT_H
