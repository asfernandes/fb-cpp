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
#include "fb-api.h"
#include "types.h"
#include "Blob.h"
#include "Attachment.h"
#include "Client.h"
#include "NumericConverter.h"
#include "CalendarConverter.h"
#include "Descriptor.h"
#include "SmartPtrs.h"
#include "Exception.h"
#include <charconv>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
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

///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Transaction;

	///
	/// Represents options used when preparing a Statement.
	///
	class StatementOptions final
	{
	public:
		///
		/// @brief Reports whether the legacy textual plan should be prefetched during prepare.
		///
		bool getPrefetchLegacyPlan() const
		{
			return prefetchLegacyPlan;
		}

		///
		/// @brief Enables or disables prefetching of the legacy textual plan at prepare time.
		/// @param value `true` to prefetch the legacy plan, `false` to skip it.
		/// @return Reference to this instance for fluent configuration.
		///
		StatementOptions& setPrefetchLegacyPlan(bool value)
		{
			prefetchLegacyPlan = value;
			return *this;
		}

		///
		/// @brief Reports whether the structured plan should be prefetched during prepare.
		///
		bool getPrefetchPlan() const
		{
			return prefetchPlan;
		}

		///
		/// @brief Enables or disables prefetching of the structured plan at prepare time.
		/// @param value `true` to prefetch the optimized plan, `false` to skip it.
		/// @return Reference to this instance for fluent configuration.
		///
		StatementOptions& setPrefetchPlan(bool value)
		{
			prefetchPlan = value;
			return *this;
		}

	private:
		bool prefetchLegacyPlan = false;
		bool prefetchPlan = false;
	};

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
	class Statement final
	{
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
		Statement(Statement&& o) noexcept
			: attachment{o.attachment},
			  status{std::move(o.status)},
			  statusWrapper{std::move(o.statusWrapper)},
			  calendarConverter{std::move(calendarConverter)},
			  numericConverter{std::move(numericConverter)},
			  statementHandle{std::move(o.statementHandle)},
			  resultSetHandle{std::move(o.resultSetHandle)},
			  inMetadata{std::move(o.inMetadata)},
			  inDescriptors{std::move(o.inDescriptors)},
			  inMessage{std::move(o.inMessage)},
			  outMetadata{std::move(o.outMetadata)},
			  outDescriptors{std::move(o.outDescriptors)},
			  outMessage{std::move(o.outMessage)},
			  type{o.type}
		{
		}

		Statement& operator=(Statement&&) = delete;
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
		/// Returns whether the Statement object is valid.
		///
		bool isValid() noexcept
		{
			return statementHandle != nullptr;
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
		/// @brief Returns the metadata describing columns produced by the statement.
		///
		FbRef<fb::IMessageMetadata> getOutputMetadata() noexcept
		{
			return outMetadata;
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
		/// @return `true` when execution yields a result set that can be fetched.
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

		///
		/// @brief Marks all bound parameters as null values.
		///
		void clearParameters()
		{
			assert(isValid());

			const auto message = inMessage.data();

			for (const auto& descriptor : inDescriptors)
				*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_TRUE;
		}

		///
		/// @brief Marks the specified parameter as null.
		/// @param index Zero-based parameter index.
		///
		void setNull(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_TRUE;
		}

		///
		/// @brief Binds a boolean parameter value or null.
		/// @param index Zero-based parameter index.
		/// @param optValue Boolean value to assign, or empty to bind null.
		///
		void setBool(unsigned index, std::optional<bool> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					message[descriptor.offset] = value ? std::byte{1} : std::byte{0};
					break;

				default:
					throwInvalidType("bool", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a 16-bit signed integer value or null.
		///
		void setInt16(unsigned index, std::optional<std::int16_t> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT16, optValue.value(), 0, "std::int16_t");
		}

		///
		/// @brief Binds a scaled 16-bit signed integer value or null.
		///
		void setScaledInt16(unsigned index, std::optional<ScaledInt16> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT16, value.value, value.scale, "ScaledInt16");
		}

		///
		/// @brief Binds a 32-bit signed integer value or null.
		///
		void setInt32(unsigned index, std::optional<std::int32_t> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT32, optValue.value(), 0, "std::int32_t");
		}

		///
		/// @brief Binds a scaled 32-bit signed integer value or null.
		///
		void setScaledInt32(unsigned index, std::optional<ScaledInt32> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT32, value.value, value.scale, "ScaledInt32");
		}

		///
		/// @brief Binds a 64-bit signed integer value or null.
		///
		void setInt64(unsigned index, std::optional<std::int64_t> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT64, optValue.value(), 0, "std::int64_t");
		}

		///
		/// @brief Binds a scaled 64-bit signed integer value or null.
		///
		void setScaledInt64(unsigned index, std::optional<ScaledInt64> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT64, value.value, value.scale, "ScaledInt64");
		}

		///
		/// @brief Binds a raw 128-bit integer value in Firebird's representation or null.
		///
		void setOpaqueInt128(unsigned index, std::optional<OpaqueInt128> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT128:
					*reinterpret_cast<OpaqueInt128*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueInt128", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Binds a 128-bit integer value expressed with Boost.Multiprecision or null.
		///
		void setBoostInt128(unsigned index, std::optional<BoostInt128> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::INT128, optValue.value(), 0, "BoostInt128");
		}

		///
		/// @brief Binds a scaled 128-bit integer value expressed with Boost.Multiprecision or null.
		///
		void setScaledBoostInt128(unsigned index, std::optional<ScaledBoostInt128> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			const auto& value = optValue.value();
			setNumber(index, DescriptorAdjustedType::INT128, value.value, value.scale, "ScaledBoostInt128");
		}
#endif

		///
		/// @brief Binds a single precision floating-point value or null.
		///
		void setFloat(unsigned index, std::optional<float> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::FLOAT, optValue.value(), 0, "float");
		}

		///
		/// @brief Binds a double precision floating-point value or null.
		///
		void setDouble(unsigned index, std::optional<double> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DOUBLE, optValue.value(), 0, "double");
		}

		///
		/// @brief Binds a 16-digit decimal floating-point value in Firebird's representation or null.
		///
		void setOpaqueDecFloat16(unsigned index, std::optional<OpaqueDecFloat16> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DECFLOAT16:
					*reinterpret_cast<OpaqueDecFloat16*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueDecFloat16", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Binds a 16-digit decimal floating-point value using Boost.Multiprecision or null.
		///
		void setBoostDecFloat16(unsigned index, std::optional<BoostDecFloat16> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DECFLOAT16, optValue.value(), 0, "BoostDecFloat16");
		}
#endif

		///
		/// @brief Binds a 34-digit decimal floating-point value in Firebird's representation or null.
		///
		void setOpaqueDecFloat34(unsigned index, std::optional<OpaqueDecFloat34> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DECFLOAT34:
					*reinterpret_cast<OpaqueDecFloat34*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueDecFloat34", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Binds a 34-digit decimal floating-point value using Boost.Multiprecision or null.
		///
		void setBoostDecFloat34(unsigned index, std::optional<BoostDecFloat34> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			setNumber(index, DescriptorAdjustedType::DECFLOAT34, optValue.value(), 0, "BoostDecFloat34");
		}
#endif

		///
		/// @brief Binds a date value or null.
		///
		void setDate(unsigned index, std::optional<Date> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DATE:
					*reinterpret_cast<OpaqueDate*>(&message[descriptor.offset]) =
						calendarConverter.dateToOpaqueDate(value);
					break;

				default:
					throwInvalidType("Date", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a raw date value in Firebird's representation or null.
		///
		void setOpaqueDate(unsigned index, std::optional<OpaqueDate> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DATE:
					*reinterpret_cast<OpaqueDate*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueDate", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a time-of-day value without timezone or null.
		///
		void setTime(unsigned index, std::optional<Time> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME:
					*reinterpret_cast<OpaqueTime*>(&message[descriptor.offset]) =
						calendarConverter.timeToOpaqueTime(value);
					break;

				default:
					throwInvalidType("Time", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a raw time-of-day value in Firebird's representation or null.
		///
		void setOpaqueTime(unsigned index, std::optional<OpaqueTime> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME:
					*reinterpret_cast<OpaqueTime*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueTime", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a timestamp value without timezone or null.
		///
		void setTimestamp(unsigned index, std::optional<Timestamp> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP:
					*reinterpret_cast<OpaqueTimestamp*>(&message[descriptor.offset]) =
						calendarConverter.timestampToOpaqueTimestamp(value);
					break;

				default:
					throwInvalidType("Timestamp", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a raw timestamp value in Firebird's representation or null.
		///
		void setOpaqueTimestamp(unsigned index, std::optional<OpaqueTimestamp> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP:
					*reinterpret_cast<OpaqueTimestamp*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueTimestamp", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a time-of-day value with timezone or null.
		///
		void setTimeTz(unsigned index, std::optional<TimeTz> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			auto* const message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME_TZ:
					*reinterpret_cast<OpaqueTimeTz*>(&message[descriptor.offset]) =
						calendarConverter.timeTzToOpaqueTimeTz(value);
					break;

				default:
					throwInvalidType("TimeTz", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a raw time-of-day value with timezone in Firebird's representation or null.
		///
		void setOpaqueTimeTz(unsigned index, std::optional<OpaqueTimeTz> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			auto* const message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME_TZ:
					*reinterpret_cast<OpaqueTimeTz*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueTimeTz", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a timestamp value with timezone or null.
		///
		void setTimestampTz(unsigned index, std::optional<TimestampTz> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			auto* const message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP_TZ:
					*reinterpret_cast<OpaqueTimestampTz*>(&message[descriptor.offset]) =
						calendarConverter.timestampTzToOpaqueTimestampTz(value);
					break;

				default:
					throwInvalidType("TimestampTz", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a raw timestamp value with timezone in Firebird's representation or null.
		///
		void setOpaqueTimestampTz(unsigned index, std::optional<OpaqueTimestampTz> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			auto* const message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP_TZ:
					*reinterpret_cast<OpaqueTimestampTz*>(&message[descriptor.offset]) = value;
					break;

				default:
					throwInvalidType("OpaqueTimestampTz", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a textual parameter or null, performing direct conversions where supported.
		///
		void setString(unsigned index, std::optional<std::string_view> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			const auto message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					message[descriptor.offset] = numericConverter.stringToBoolean(value);
					break;

				// FIXME: scale
				case DescriptorAdjustedType::INT16:
				case DescriptorAdjustedType::INT32:
				case DescriptorAdjustedType::INT64:
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
#endif
				{
					std::string strValue(value);
					int scale = 0;

					if (const auto dotPos = strValue.find_last_of('.'); dotPos != std::string_view::npos)
					{
						for (auto pos = dotPos + 1; pos < strValue.size(); ++pos)
						{
							const char c = value[pos];

							if (c < '0' || c > '9')
								break;

							--scale;
						}

						strValue.erase(dotPos, 1);
					}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
					// FIXME: try/rethrow when parsing
					// FIXME: review
					const auto parseDecimalToBoostInt128 = [this](std::string_view text)
					{
						bool isNegative = false;
						std::size_t pos = 0;

						if (!text.empty() && (text.front() == '+' || text.front() == '-'))
						{
							isNegative = text.front() == '-';
							++pos;
						}

						if (pos == text.size())
							this->numericConverter.throwNumericOutOfRange();  // FIXME: conversion error

						BoostInt128 result{};

						for (; pos < text.size(); ++pos)
						{
							const char c = text[pos];

							if (c < '0' || c > '9')
								this->numericConverter.throwNumericOutOfRange();  // FIXME: conversion error

							result *= 10;
							result += static_cast<int>(c - '0');
						}

						return isNegative ? -result : result;
					};

					auto scaledValue = ScaledBoostInt128{parseDecimalToBoostInt128(strValue), scale};

					if (scale != descriptor.scale)
					{
						scaledValue.value = numericConverter.numberToNumber<BoostInt128>(scaledValue, descriptor.scale);
						scaledValue.scale = descriptor.scale;
					}

					setScaledBoostInt128(index, scaledValue);
#else
					static_assert(sizeof(long long) == sizeof(std::int64_t));
					std::int64_t intValue;
					const auto convResult =
						std::from_chars(strValue.data(), strValue.data() + strValue.size(), intValue);
					if (convResult.ec != std::errc{} || convResult.ptr != strValue.data() + strValue.size())
						numericConverter.throwNumericOutOfRange();  // FIXME: conversion error
					auto scaledValue = ScaledInt64{intValue, scale};

					if (scale != descriptor.scale)
					{
						scaledValue.value =
							numericConverter.numberToNumber<std::int64_t>(scaledValue, descriptor.scale);
						scaledValue.scale = descriptor.scale;
					}

					setScaledInt64(index, scaledValue);
#endif
					return;
				}

				case DescriptorAdjustedType::FLOAT:
				case DescriptorAdjustedType::DOUBLE:
				{
					double doubleValue;
					const auto convResult = std::from_chars(value.data(), value.data() + value.size(), doubleValue);
					if (convResult.ec != std::errc{} || convResult.ptr != value.data() + value.size())
						numericConverter.throwNumericOutOfRange();  // FIXME: conversion error
					setDouble(index, doubleValue);
					return;
				}

				case DescriptorAdjustedType::DATE:
					*reinterpret_cast<OpaqueDate*>(&message[descriptor.offset]) =
						calendarConverter.stringToOpaqueDate(value);
					break;

				case DescriptorAdjustedType::TIME:
					*reinterpret_cast<OpaqueTime*>(&message[descriptor.offset]) =
						calendarConverter.stringToOpaqueTime(value);
					break;

				case DescriptorAdjustedType::TIMESTAMP:
					*reinterpret_cast<OpaqueTimestamp*>(&message[descriptor.offset]) =
						calendarConverter.stringToOpaqueTimestamp(value);
					break;

				case DescriptorAdjustedType::TIME_TZ:
					*reinterpret_cast<OpaqueTimeTz*>(&message[descriptor.offset]) =
						calendarConverter.stringToOpaqueTimeTz(value);
					break;

				case DescriptorAdjustedType::TIMESTAMP_TZ:
					*reinterpret_cast<OpaqueTimestampTz*>(&message[descriptor.offset]) =
						calendarConverter.stringToOpaqueTimestampTz(value);
					break;
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::DECFLOAT16:
				case DescriptorAdjustedType::DECFLOAT34:
					// FIXME: try/rethrow when parsing
					setBoostDecFloat34(index, BoostDecFloat34{value});
					return;
#endif

				case DescriptorAdjustedType::STRING:
					if (value.length() > descriptor.length)
					{
						static constexpr std::intptr_t STATUS_STRING_TRUNCATION[] = {
							isc_arith_except,
							isc_string_truncation,
							isc_arg_end,
						};

						throw DatabaseException(attachment.getClient(), STATUS_STRING_TRUNCATION);
					}

					*reinterpret_cast<std::uint16_t*>(&message[descriptor.offset]) =
						static_cast<std::uint16_t>(value.length());
					std::copy(value.begin(), value.end(),
						reinterpret_cast<char*>(&message[descriptor.offset + sizeof(std::uint16_t)]));
					break;

				default:
					throwInvalidType("std::string_view", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		///
		/// @brief Binds a blob identifier to the specified parameter or null.
		///
		void setBlobId(unsigned index, std::optional<BlobId> optValue)
		{
			if (!optValue.has_value())
			{
				setNull(index);
				return;
			}

			assert(isValid());

			const auto& value = optValue.value();
			const auto& descriptor = getInDescriptor(index);
			auto* const message = inMessage.data();

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BLOB:
					*reinterpret_cast<ISC_QUAD*>(&message[descriptor.offset]) = value.id;
					break;

				default:
					throwInvalidType("BlobId", descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		/// @brief Convenience overload that binds a null value.
		///
		void set(unsigned index, std::nullopt_t)
		{
			setNull(index);
		}

		///
		/// @brief Convenience overload that binds a blob identifier.
		///
		void set(unsigned index, BlobId value)
		{
			setBlobId(index, value);
		}

		///
		/// @brief Convenience overload that binds an optional blob identifier.
		///
		void set(unsigned index, std::optional<BlobId> value)
		{
			setBlobId(index, value);
		}

		///
		/// @brief Convenience overload that binds a boolean value.
		///
		void set(unsigned index, bool value)
		{
			setBool(index, value);
		}

		///
		/// @brief Convenience overload that binds a 16-bit signed integer.
		///
		void set(unsigned index, std::int16_t value)
		{
			setInt16(index, value);
		}

		///
		/// @brief Convenience overload that binds a scaled 16-bit signed integer.
		///
		void set(unsigned index, ScaledInt16 value)
		{
			setScaledInt16(index, value);
		}

		///
		/// @brief Convenience overload that binds a 32-bit signed integer.
		///
		void set(unsigned index, std::int32_t value)
		{
			setInt32(index, value);
		}

		///
		/// @brief Convenience overload that binds a scaled 32-bit signed integer.
		///
		void set(unsigned index, ScaledInt32 value)
		{
			setScaledInt32(index, value);
		}

		///
		/// @brief Convenience overload that binds a 64-bit signed integer.
		///
		void set(unsigned index, std::int64_t value)
		{
			setInt64(index, value);
		}

		///
		/// @brief Convenience overload that binds a scaled 64-bit signed integer.
		///
		void set(unsigned index, ScaledInt64 value)
		{
			setScaledInt64(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird 128-bit integer.
		///
		void set(unsigned index, OpaqueInt128 value)
		{
			setOpaqueInt128(index, value);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Convenience overload that binds a Boost-provided 128-bit integer.
		///
		void set(unsigned index, BoostInt128 value)
		{
			setBoostInt128(index, value);
		}

		///
		/// @brief Convenience overload that binds a scaled Boost-provided 128-bit integer.
		///
		void set(unsigned index, ScaledBoostInt128 value)
		{
			setScaledBoostInt128(index, value);
		}
#endif

		///
		/// @brief Convenience overload that binds a single precision floating-point value.
		///
		void set(unsigned index, float value)
		{
			setFloat(index, value);
		}

		///
		/// @brief Convenience overload that binds a double precision floating-point value.
		///
		void set(unsigned index, double value)
		{
			setDouble(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird 16-digit decimal floating-point value.
		///
		void set(unsigned index, OpaqueDecFloat16 value)
		{
			setOpaqueDecFloat16(index, value);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Convenience overload that binds a Boost 16-digit decimal floating-point value.
		///
		void set(unsigned index, BoostDecFloat16 value)
		{
			setBoostDecFloat16(index, value);
		}
#endif

		///
		/// @brief Convenience overload that binds a Firebird 34-digit decimal floating-point value.
		///
		void set(unsigned index, OpaqueDecFloat34 value)
		{
			setOpaqueDecFloat34(index, value);
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Convenience overload that binds a Boost 34-digit decimal floating-point value.
		///
		void set(unsigned index, BoostDecFloat34 value)
		{
			setBoostDecFloat34(index, value);
		}
#endif

		///
		/// @brief Convenience overload that binds a Firebird date value.
		///
		void set(unsigned index, Date value)
		{
			setDate(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird date value.
		///
		void set(unsigned index, OpaqueDate value)
		{
			setOpaqueDate(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird time value.
		///
		void set(unsigned index, Time value)
		{
			setTime(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird time value.
		///
		void set(unsigned index, OpaqueTime value)
		{
			setOpaqueTime(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird timestamp value.
		///
		void set(unsigned index, Timestamp value)
		{
			setTimestamp(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird timestamp value.
		///
		void set(unsigned index, OpaqueTimestamp value)
		{
			setOpaqueTimestamp(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird time with timezone value.
		///
		void set(unsigned index, TimeTz value)
		{
			setTimeTz(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird time with timezone value.
		///
		void set(unsigned index, OpaqueTimeTz value)
		{
			setOpaqueTimeTz(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird timestamp with timezone value.
		///
		void set(unsigned index, TimestampTz value)
		{
			setTimestampTz(index, value);
		}

		///
		/// @brief Convenience overload that binds a Firebird timestamp with timezone value.
		///
		void set(unsigned index, OpaqueTimestampTz value)
		{
			setOpaqueTimestampTz(index, value);
		}

		///
		/// @brief Convenience overload that binds a textual value.
		///
		void set(unsigned index, std::string_view value)
		{
			setString(index, value);
		}

		///
		/// @brief Convenience template that forwards optional values to specialized overloads.
		///
		template <typename T>
		void set(unsigned index, std::optional<T> value)
		{
			if (value.has_value())
				set(index, value.value());
			else
				setNull(index);
		}

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

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			return *reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE;
		}

		///
		/// @brief Reads a boolean column from the current row.
		///
		std::optional<bool> getBool(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					return message[descriptor.offset] != std::byte{0};

				default:
					throwInvalidType("bool", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a 16-bit signed integer column.
		///
		std::optional<std::int16_t> getInt16(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<std::int16_t>(index, scale, "std::int16_t");
		}

		///
		/// @brief Reads a scaled 16-bit signed integer column.
		///
		std::optional<ScaledInt16> getScaledInt16(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<std::int16_t>(index, scale, "ScaledInt16");
			return value.has_value() ? std::optional{ScaledInt16{value.value(), scale.value()}} : std::nullopt;
		}

		///
		/// @brief Reads a 32-bit signed integer column.
		///
		std::optional<std::int32_t> getInt32(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<std::int32_t>(index, scale, "std::int32_t");
		}

		///
		/// @brief Reads a scaled 32-bit signed integer column.
		///
		std::optional<ScaledInt32> getScaledInt32(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<std::int32_t>(index, scale, "ScaledInt32");
			return value.has_value() ? std::optional{ScaledInt32{value.value(), scale.value()}} : std::nullopt;
		}

		///
		/// @brief Reads a 64-bit signed integer column.
		///
		std::optional<std::int64_t> getInt64(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<std::int64_t>(index, scale, "std::int64_t");
		}

		///
		/// @brief Reads a scaled 64-bit signed integer column.
		///
		std::optional<ScaledInt64> getScaledInt64(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<std::int64_t>(index, scale, "ScaledInt64");
			return value.has_value() ? std::optional{ScaledInt64{value.value(), scale.value()}} : std::nullopt;
		}

		///
		/// @brief Reads a Firebird 128-bit integer column.
		///
		std::optional<ScaledOpaqueInt128> getScaledOpaqueInt128(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT128:
					return ScaledOpaqueInt128{
						OpaqueInt128{*reinterpret_cast<const OpaqueInt128*>(&message[descriptor.offset])},
						descriptor.scale};

				default:
					throwInvalidType("OpaqueInt128", descriptor.adjustedType);
			}
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost 128-bit integer column.
		///
		std::optional<BoostInt128> getBoostInt128(unsigned index)
		{
			std::optional<int> scale{0};
			const auto value = getNumber<BoostInt128>(index, scale, "BoostInt128");
			return value.has_value() ? std::optional{value.value()} : std::nullopt;
		}

		///
		/// @brief Reads a scaled Boost 128-bit integer column.
		///
		std::optional<ScaledBoostInt128> getScaledBoostInt128(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<BoostInt128>(index, scale, "ScaledBoostInt128");
			return value.has_value() ? std::optional{ScaledBoostInt128{value.value(), scale.value()}} : std::nullopt;
		}
#endif

		///
		/// @brief Reads a single precision floating-point column.
		///
		std::optional<float> getFloat(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<float>(index, scale, "float");
		}

		///
		/// @brief Reads a double precision floating-point column.
		///
		std::optional<double> getDouble(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<double>(index, scale, "double");
		}

		///
		/// @brief Reads a Firebird 16-digit decimal floating-point column.
		///
		std::optional<OpaqueDecFloat16> getOpaqueDecFloat16(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DECFLOAT16:
					return OpaqueDecFloat16{*reinterpret_cast<const OpaqueDecFloat16*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueDecFloat16", descriptor.adjustedType);
			}
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost-based 16-digit decimal floating-point column.
		///
		std::optional<BoostDecFloat16> getBoostDecFloat16(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<BoostDecFloat16>(index, scale, "BoostDecFloat16");
		}
#endif

		///
		/// @brief Reads a Firebird 34-digit decimal floating-point column.
		///
		std::optional<OpaqueDecFloat34> getOpaqueDecFloat34(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DECFLOAT34:
					return OpaqueDecFloat34{*reinterpret_cast<const OpaqueDecFloat34*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueDecFloat34", descriptor.adjustedType);
			}
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost-based 34-digit decimal floating-point column.
		///
		std::optional<BoostDecFloat34> getBoostDecFloat34(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<BoostDecFloat34>(index, scale, "BoostDecFloat34");
		}
#endif

		///
		/// @brief Reads a date column.
		///
		std::optional<Date> getDate(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DATE:
					return calendarConverter.opaqueDateToDate(
						*reinterpret_cast<const OpaqueDate*>(&message[descriptor.offset]));

				default:
					throwInvalidType("Date", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw date column in Firebird's representation.
		///
		std::optional<OpaqueDate> getOpaqueDate(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DATE:
					return OpaqueDate{*reinterpret_cast<const OpaqueDate*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueDate", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a time-of-day column without timezone.
		///
		std::optional<Time> getTime(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME:
					return calendarConverter.opaqueTimeToTime(
						*reinterpret_cast<const OpaqueTime*>(&message[descriptor.offset]));

				default:
					throwInvalidType("Time", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw time-of-day column in Firebird's representation.
		///
		std::optional<OpaqueTime> getOpaqueTime(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME:
					return OpaqueTime{*reinterpret_cast<const OpaqueTime*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTime", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a timestamp column without timezone.
		///
		std::optional<Timestamp> getTimestamp(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP:
					return calendarConverter.opaqueTimestampToTimestamp(
						*reinterpret_cast<const OpaqueTimestamp*>(&message[descriptor.offset]));

				default:
					throwInvalidType("Timestamp", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw timestamp column in Firebird's representation.
		///
		std::optional<OpaqueTimestamp> getOpaqueTimestamp(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP:
					return OpaqueTimestamp{*reinterpret_cast<const OpaqueTimestamp*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTimestamp", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a time-of-day column with timezone.
		///
		std::optional<TimeTz> getTimeTz(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME_TZ:
					return calendarConverter.opaqueTimeTzToTimeTz(
						*reinterpret_cast<const OpaqueTimeTz*>(&message[descriptor.offset]));

				default:
					throwInvalidType("TimeTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw time-of-day column with timezone in Firebird's representation.
		///
		std::optional<OpaqueTimeTz> getOpaqueTimeTz(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME_TZ:
					return OpaqueTimeTz{*reinterpret_cast<const OpaqueTimeTz*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTimeTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a timestamp-with-time-zone column.
		///
		std::optional<TimestampTz> getTimestampTz(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP_TZ:
					return calendarConverter.opaqueTimestampTzToTimestampTz(
						*reinterpret_cast<const OpaqueTimestampTz*>(&message[descriptor.offset]));

				default:
					throwInvalidType("TimestampTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw timestamp-with-time-zone column in Firebird's representation.
		///
		std::optional<OpaqueTimestampTz> getOpaqueTimestampTz(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP_TZ:
					return OpaqueTimestampTz{*reinterpret_cast<const OpaqueTimestampTz*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTimestampTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a blob identifier column.
		///
		std::optional<BlobId> getBlobId(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BLOB:
				{
					BlobId value;
					value.id = *reinterpret_cast<const ISC_QUAD*>(&message[descriptor.offset]);
					return value;
				}

				default:
					throwInvalidType("BlobId", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a textual column, applying number-to-string conversions when needed.
		///
		std::optional<std::string> getString(unsigned index)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			const auto data = &message[descriptor.offset];

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					return (message[descriptor.offset] != std::byte{0}) ? std::string{"true"} : std::string{"false"};

				case DescriptorAdjustedType::INT16:
					return numericConverter.numberToString(
						ScaledInt16{*reinterpret_cast<const std::int16_t*>(data), descriptor.scale});

				case DescriptorAdjustedType::INT32:
					return numericConverter.numberToString(
						ScaledInt32{*reinterpret_cast<const std::int32_t*>(data), descriptor.scale});

				case DescriptorAdjustedType::INT64:
					return numericConverter.numberToString(
						ScaledInt64{*reinterpret_cast<const std::int64_t*>(data), descriptor.scale});

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
					return numericConverter.numberToString(ScaledBoostInt128{
						numericConverter.opaqueInt128ToBoostInt128(*reinterpret_cast<const OpaqueInt128*>(data)),
						descriptor.scale});
#endif

				case DescriptorAdjustedType::FLOAT:
					return numericConverter.numberToString(*reinterpret_cast<const float*>(data));

				case DescriptorAdjustedType::DOUBLE:
					return numericConverter.numberToString(*reinterpret_cast<const double*>(data));

				case DescriptorAdjustedType::DATE:
					return calendarConverter.opaqueDateToString(*reinterpret_cast<const OpaqueDate*>(data));

				case DescriptorAdjustedType::TIME:
					return calendarConverter.opaqueTimeToString(*reinterpret_cast<const OpaqueTime*>(data));

				case DescriptorAdjustedType::TIMESTAMP:
					return calendarConverter.opaqueTimestampToString(*reinterpret_cast<const OpaqueTimestamp*>(data));

				case DescriptorAdjustedType::TIME_TZ:
					return calendarConverter.opaqueTimeTzToString(*reinterpret_cast<const OpaqueTimeTz*>(data));

				case DescriptorAdjustedType::TIMESTAMP_TZ:
					return calendarConverter.opaqueTimestampTzToString(
						*reinterpret_cast<const OpaqueTimestampTz*>(data));

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::DECFLOAT16:
					return numericConverter.numberToString(numericConverter.opaqueDecFloat16ToBoostDecFloat16(
						*reinterpret_cast<const OpaqueDecFloat16*>(data)));

				case DescriptorAdjustedType::DECFLOAT34:
					return numericConverter.numberToString(numericConverter.opaqueDecFloat34ToBoostDecFloat34(
						*reinterpret_cast<const OpaqueDecFloat34*>(data)));
#endif

				case DescriptorAdjustedType::STRING:
					return std::string{reinterpret_cast<const char*>(data + sizeof(std::uint16_t)),
						*reinterpret_cast<const std::uint16_t*>(data)};

				default:
					throwInvalidType("std::string", descriptor.adjustedType);
			}
		}

		///
		/// @}
		///

		///
		/// @brief Retrieves a column using the most appropriate typed accessor specialization.
		///
		template <typename T>
		T get(unsigned index);

	private:
		///
		/// @brief Validates and returns the descriptor for the given input parameter index.
		///
		const Descriptor& getInDescriptor(unsigned index)
		{
			if (index >= inDescriptors.size())
				throw std::out_of_range("index out of range");

			return inDescriptors[index];
		}

		///
		/// @brief Validates and returns the descriptor for the given output column index.
		///
		const Descriptor& getOutDescriptor(unsigned index)
		{
			if (index >= outDescriptors.size())
				throw std::out_of_range("index out of range");

			return outDescriptors[index];
		}

		// FIXME: typeName unneeded or incorrect?
		///
		/// @brief Converts and writes numeric parameter values following descriptor rules.
		///
		template <typename T>
		void setNumber(unsigned index, DescriptorAdjustedType valueType, T value, int scale, const char* typeName)
		{
			assert(isValid());

			const auto& descriptor = getInDescriptor(index);
			auto* const message = inMessage.data();

			const auto descriptorData = &message[descriptor.offset];
			std::optional<int> descriptorScale{descriptor.scale};

			Descriptor valueDescriptor;
			valueDescriptor.adjustedType = valueType;
			valueDescriptor.scale = scale;

			const auto valueAddress = reinterpret_cast<const std::byte*>(&value);

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT16:
					*reinterpret_cast<std::int16_t*>(descriptorData) =
						convertNumber<std::int16_t>(valueDescriptor, valueAddress, descriptorScale, "std::int16_t");
					break;

				case DescriptorAdjustedType::INT32:
					*reinterpret_cast<std::int32_t*>(descriptorData) =
						convertNumber<std::int32_t>(valueDescriptor, valueAddress, descriptorScale, "std::int32_t");
					break;

				case DescriptorAdjustedType::INT64:
					*reinterpret_cast<std::int64_t*>(descriptorData) =
						convertNumber<std::int64_t>(valueDescriptor, valueAddress, descriptorScale, "std::int64_t");
					break;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
				{
					const auto boostInt128 =
						convertNumber<BoostInt128>(valueDescriptor, valueAddress, descriptorScale, "BoostInt128");
					*reinterpret_cast<OpaqueInt128*>(descriptorData) =
						numericConverter.boostInt128ToOpaqueInt128(boostInt128);
					break;
				}
#endif

				case DescriptorAdjustedType::FLOAT:
					*reinterpret_cast<float*>(descriptorData) =
						convertNumber<float>(valueDescriptor, valueAddress, descriptorScale, "float");
					break;

				case DescriptorAdjustedType::DOUBLE:
					*reinterpret_cast<double*>(descriptorData) =
						convertNumber<double>(valueDescriptor, valueAddress, descriptorScale, "double");
					break;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::DECFLOAT16:
				{
					const auto boostDecFloat16 = convertNumber<BoostDecFloat16>(
						valueDescriptor, valueAddress, descriptorScale, "BoostDecFloat16");
					*reinterpret_cast<OpaqueDecFloat16*>(descriptorData) =
						numericConverter.boostDecFloat16ToOpaqueDecFloat16(boostDecFloat16);
					break;
				}

				case DescriptorAdjustedType::DECFLOAT34:
				{
					const auto boostDecFloat34 = convertNumber<BoostDecFloat34>(
						valueDescriptor, valueAddress, descriptorScale, "BoostDecFloat34");
					*reinterpret_cast<OpaqueDecFloat34*>(descriptorData) =
						numericConverter.boostDecFloat34ToOpaqueDecFloat34(boostDecFloat34);
					break;
				}
#endif

				default:
					throwInvalidType(typeName, descriptor.adjustedType);
			}

			*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_FALSE;
		}

		// FIXME: floating to integral
		///
		/// @brief Reads numeric column data, performing conversion to the desired type.
		///
		template <typename T>
		std::optional<T> getNumber(unsigned index, std::optional<int>& scale, const char* typeName)
		{
			assert(isValid());

			const auto& descriptor = getOutDescriptor(index);
			const auto* const message = outMessage.data();

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			auto data = &message[descriptor.offset];
			std::optional<BoostInt128> boostInt128;
			std::optional<BoostDecFloat16> boostDecFloat16;
			std::optional<BoostDecFloat34> boostDecFloat34;

			switch (descriptor.adjustedType)
			{
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
					boostInt128.emplace(
						numericConverter.opaqueInt128ToBoostInt128(*reinterpret_cast<const OpaqueInt128*>(data)));
					data = reinterpret_cast<const std::byte*>(&boostInt128.value());
					break;

				case DescriptorAdjustedType::DECFLOAT16:
					boostDecFloat16.emplace(numericConverter.opaqueDecFloat16ToBoostDecFloat16(
						*reinterpret_cast<const OpaqueDecFloat16*>(data)));
					data = reinterpret_cast<const std::byte*>(&boostDecFloat16.value());
					break;

				case DescriptorAdjustedType::DECFLOAT34:
					boostDecFloat34.emplace(numericConverter.opaqueDecFloat34ToBoostDecFloat34(
						*reinterpret_cast<const OpaqueDecFloat34*>(data)));
					data = reinterpret_cast<const std::byte*>(&boostDecFloat34.value());
					break;
#endif

				default:
					break;
			}

			return convertNumber<T>(descriptor, data, scale, typeName);
		}

		[[noreturn]] static void throwInvalidType(const char* actualType, DescriptorAdjustedType descriptorType)
		{
			throw FbCppException("Invalid type: actual type " + std::string(actualType) + ", descriptor type " +
				std::to_string(static_cast<unsigned>(descriptorType)));
		}

		template <typename T>
		T convertNumber(
			const Descriptor& descriptor, const std::byte* data, std::optional<int>& toScale, const char* toTypeName)
		{
			if (!toScale.has_value())  // FIXME: ?
			{
				switch (descriptor.adjustedType)
				{
					case DescriptorAdjustedType::DECFLOAT16:
					case DescriptorAdjustedType::DECFLOAT34:
					case DescriptorAdjustedType::FLOAT:
					case DescriptorAdjustedType::DOUBLE:
						throwInvalidType(toTypeName, descriptor.adjustedType);

					default:
						break;
				}

				toScale = descriptor.scale;
			}

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT16:
					return numericConverter.numberToNumber<T>(
						ScaledInt16{*reinterpret_cast<const std::int16_t*>(data), descriptor.scale}, toScale.value());
					break;

				case DescriptorAdjustedType::INT32:
					return numericConverter.numberToNumber<T>(
						ScaledInt32{*reinterpret_cast<const std::int32_t*>(data), descriptor.scale}, toScale.value());

				case DescriptorAdjustedType::INT64:
					return numericConverter.numberToNumber<T>(
						ScaledInt64{*reinterpret_cast<const std::int64_t*>(data), descriptor.scale}, toScale.value());

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
					return numericConverter.numberToNumber<T>(
						ScaledBoostInt128{*reinterpret_cast<const BoostInt128*>(data), descriptor.scale},
						toScale.value());

				case DescriptorAdjustedType::DECFLOAT16:
					return numericConverter.numberToNumber<T>(
						*reinterpret_cast<const BoostDecFloat16*>(data), toScale.value());

				case DescriptorAdjustedType::DECFLOAT34:
					return numericConverter.numberToNumber<T>(
						*reinterpret_cast<const BoostDecFloat34*>(data), toScale.value());
#endif

				case DescriptorAdjustedType::FLOAT:
					return numericConverter.numberToNumber<T>(*reinterpret_cast<const float*>(data), toScale.value());
					break;

				case DescriptorAdjustedType::DOUBLE:
					return numericConverter.numberToNumber<T>(*reinterpret_cast<const double*>(data), toScale.value());
					break;

				default:
					throwInvalidType(toTypeName, descriptor.adjustedType);
			}
		}

	private:
		Attachment& attachment;
		FbUniquePtr<Firebird::IStatus> status;
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
		StatementType type;
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

	///
	/// @}
	///
}  // namespace fbcpp


#endif  // FBCPP_STATEMENT_H
