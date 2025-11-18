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

#ifndef FBCPP_DESCRIPTOR_H
#define FBCPP_DESCRIPTOR_H

#include "fb-api.h"


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Descriptor original type.
	///
	enum class DescriptorOriginalType : unsigned
	{
		NULL_TYPE = SQL_NULL,
		TEXT = SQL_TEXT,
		VARYING = SQL_VARYING,
		SHORT = SQL_SHORT,
		LONG = SQL_LONG,
		FLOAT = SQL_FLOAT,
		DOUBLE = SQL_DOUBLE,
		TIMESTAMP = SQL_TIMESTAMP,
		BLOB = SQL_BLOB,
		TIME = SQL_TYPE_TIME,
		DATE = SQL_TYPE_DATE,
		INT64 = SQL_INT64,
		TIMESTAMP_TZ = SQL_TIMESTAMP_TZ,
		TIMESTAMP_TZ_EX = SQL_TIMESTAMP_TZ_EX,
		TIME_TZ = SQL_TIME_TZ,
		TIME_TZ_EX = SQL_TIME_TZ_EX,
		INT128 = SQL_INT128,
		DEC16 = SQL_DEC16,
		DEC34 = SQL_DEC34,
		BOOLEAN = SQL_BOOLEAN,
	};

	///
	/// Descriptor adjusted type.
	///
	enum class DescriptorAdjustedType : unsigned
	{
		NULL_TYPE = SQL_NULL,
		STRING = SQL_VARYING,
		INT16 = SQL_SHORT,
		INT32 = SQL_LONG,
		FLOAT = SQL_FLOAT,
		DOUBLE = SQL_DOUBLE,
		TIMESTAMP = SQL_TIMESTAMP,
		BLOB = SQL_BLOB,
		TIME = SQL_TYPE_TIME,
		DATE = SQL_TYPE_DATE,
		INT64 = SQL_INT64,
		TIMESTAMP_TZ = SQL_TIMESTAMP_TZ,
		TIMESTAMP_TZ_EX = SQL_TIMESTAMP_TZ_EX,
		TIME_TZ = SQL_TIME_TZ,
		TIME_TZ_EX = SQL_TIME_TZ_EX,
		INT128 = SQL_INT128,
		DECFLOAT16 = SQL_DEC16,
		DECFLOAT34 = SQL_DEC34,
		BOOLEAN = SQL_BOOLEAN,
	};

	///
	/// Describes a parameter or column.
	///
	struct Descriptor final
	{
		DescriptorOriginalType originalType;
		DescriptorAdjustedType adjustedType;
		int scale;
		unsigned length;
		unsigned offset;
		unsigned nullOffset;
		bool isNullable;
		// FIXME: more things
	};
}  // namespace fbcpp


#endif  // FBCPP_DESCRIPTOR_H
