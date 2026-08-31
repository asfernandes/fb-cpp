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

#ifndef FBCPP_ARRAY_H
#define FBCPP_ARRAY_H

#include "fb-cpp-export.h"
#include "fb-api.h"
#include "Descriptor.h"
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Attachment;
	class Transaction;

	///
	/// Represents a Firebird array identifier.
	///
	class ArrayId final
	{
	public:
		/// Returns whether this array identifier is empty.
		bool isEmpty() const noexcept
		{
			return id.gds_quad_high == 0 && id.gds_quad_low == 0;
		}

	public:
		/// Stores the raw Firebird array identifier value.
		ISC_QUAD id{0, 0};
	};

	///
	/// Defines one dimension of a Firebird array.
	///
	struct ArrayBound final
	{
		/// Inclusive lower bound.
		short lower;

		/// Inclusive upper bound.
		short upper;
	};

	///
	/// Defines the element representation and bounds of a Firebird array.
	///
	struct ArrayDescriptor final
	{
		/// Relation containing this array field; required when creating a new array.
		std::string relation;

		/// Array field name; required when creating a new array.
		std::string field;

		/// SQL type of each array element.
		DescriptorOriginalType elementType;

		/// Decimal scale of each array element.
		int scale;

		/// Size in bytes of each array element in Firebird's wire representation.
		unsigned elementLength;

		/// Inclusive bounds for each dimension, from outermost to innermost.
		std::vector<ArrayBound> bounds;
	};

	///
	/// Provides slice access to a Firebird array.
	///
	class FB_CPP_EXPORT Array final
	{
	public:
		/// Creates an array helper for a new array or an existing array identifier.
		Array(Attachment& attachment, Transaction& transaction, ArrayDescriptor descriptor, ArrayId id = {});

		Array(const Array&) = delete;
		Array& operator=(const Array&) = delete;
		Array(Array&&) = delete;
		Array& operator=(Array&&) = delete;

	public:
		/// Returns the current array identifier.
		const ArrayId& getId() const noexcept
		{
			return id;
		}

		/// Returns the descriptor used for slice operations.
		const ArrayDescriptor& getDescriptor() const noexcept
		{
			return descriptor;
		}

		/// Returns the exact number of bytes in a complete array slice.
		std::size_t getSliceLength() const;

		/// Reads the complete array slice into Firebird-format bytes.
		unsigned read(std::span<std::byte> buffer);

		/// Writes the complete array slice from Firebird-format bytes.
		void write(std::span<const std::byte> buffer);

		/// Reads the complete array slice into elements matching the Firebird element representation.
		template <typename T, std::size_t Extent>
		requires(!std::is_same_v<std::remove_cv_t<T>, std::byte>)
		unsigned read(std::span<T, Extent> buffer)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			return read(std::as_writable_bytes(buffer));
		}

		/// Writes the complete array slice from elements matching the Firebird element representation.
		template <typename T, std::size_t Extent>
		requires(!std::is_same_v<std::remove_cv_t<T>, std::byte>)
		void write(std::span<const T, Extent> buffer)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			write(std::as_bytes(buffer));
		}

	private:
		void validateBuffer(std::size_t size) const;
		ISC_ARRAY_DESC buildIscDescriptor() const;

	private:
		Attachment& attachment;
		Transaction& transaction;
		ArrayDescriptor descriptor;
		ArrayId id;
	};
}  // namespace fbcpp


#endif  // FBCPP_ARRAY_H
