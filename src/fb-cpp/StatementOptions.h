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

#ifndef FBCPP_STATEMENT_OPTIONS_H
#define FBCPP_STATEMENT_OPTIONS_H

#include "fb-api.h"
#include <optional>
#include <string>

///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// @brief Selects the cursor type for a SELECT statement.
	///
	enum class CursorType
	{
		///
		/// Forward-only traversal (default, more efficient for streaming).
		///
		FORWARD_ONLY,

		///
		/// Allows bidirectional traversal and absolute/relative positioning.
		///
		SCROLLABLE,
	};

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

		///
		/// @brief Returns the cursor name to be set for the statement.
		///
		const std::optional<std::string>& getCursorName() const
		{
			return cursorName;
		}

		///
		/// @brief Sets the cursor name for the statement.
		/// @param value The name of the cursor.
		/// @return Reference to this instance for fluent configuration.
		///
		StatementOptions& setCursorName(const std::string& value)
		{
			cursorName = value;
			return *this;
		}

		///
		/// @brief Returns the cursor type to be used when opening a result set.
		///
		CursorType getCursorType() const
		{
			return cursorType;
		}

		///
		/// @brief Sets the cursor type used when opening a result set.
		/// @param value `FORWARD_ONLY` for streaming access, `SCROLLABLE` for bidirectional navigation.
		/// @return Reference to this instance for fluent configuration.
		///
		StatementOptions& setCursorType(CursorType value)
		{
			cursorType = value;
			return *this;
		}

		///
		/// @brief Returns the SQL dialect used when preparing the statement.
		///
		unsigned getDialect() const
		{
			return dialect;
		}

		///
		/// @brief Sets the SQL dialect used when preparing the statement.
		/// @param value SQL dialect number (1 for InterBase compatibility, 3 for current).
		/// @return Reference to this instance for fluent configuration.
		///
		StatementOptions& setDialect(unsigned value)
		{
			dialect = value;
			return *this;
		}

	private:
		bool prefetchLegacyPlan = false;
		bool prefetchPlan = false;
		std::optional<std::string> cursorName;
		CursorType cursorType = CursorType::FORWARD_ONLY;
		unsigned dialect = SQL_DIALECT_CURRENT;
	};
}  // namespace fbcpp

#endif  // FBCPP_STATEMENT_OPTIONS_H
