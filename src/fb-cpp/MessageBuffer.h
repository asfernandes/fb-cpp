/*
 * MIT License
 *
 * Copyright (c) 2026 F.D.Castel
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

#ifndef FBCPP_MESSAGE_BUFFER_H
#define FBCPP_MESSAGE_BUFFER_H

#include <cstddef>
#include <new>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Alignment, in bytes, of the start of every message buffer and of every row inside
	/// a RowSet buffer. It is enough for any field of a Firebird message.
	///
	inline constexpr std::size_t MESSAGE_BUFFER_ALIGNMENT = 16u;

	namespace impl
	{
		template <typename T, std::size_t Alignment>
		struct AlignedAllocator
		{
			using value_type = T;

			template <typename U>
			struct rebind
			{
				using other = AlignedAllocator<U, Alignment>;
			};

			AlignedAllocator() noexcept = default;

			template <typename U>
			AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept
			{
			}

			T* allocate(std::size_t n)
			{
				return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t{Alignment}));
			}

			void deallocate(T* p, std::size_t) noexcept
			{
				::operator delete(p, std::align_val_t{Alignment});
			}

			template <typename U>
			bool operator==(const AlignedAllocator<U, Alignment>&) const noexcept
			{
				return true;
			}
		};

		///
		/// Rounds `length` up to a multiple of MESSAGE_BUFFER_ALIGNMENT.
		///
		constexpr unsigned alignMessageLength(unsigned length) noexcept
		{
			constexpr auto alignment = static_cast<unsigned>(MESSAGE_BUFFER_ALIGNMENT);
			return (length + alignment - 1u) & ~(alignment - 1u);
		}
	}  // namespace impl

	///
	/// Byte buffer holding Firebird messages, whose start is aligned to
	/// MESSAGE_BUFFER_ALIGNMENT regardless of the global allocator.
	///
	using MessageBuffer = std::vector<std::byte, impl::AlignedAllocator<std::byte, MESSAGE_BUFFER_ALIGNMENT>>;
}  // namespace fbcpp


#endif  // FBCPP_MESSAGE_BUFFER_H
