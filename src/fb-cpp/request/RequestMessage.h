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

#ifndef FBCPP_REQUEST_MESSAGE_H
#define FBCPP_REQUEST_MESSAGE_H

#include "../config.h"
#include "fb-cpp-export.h"
#include "../fb-api.h"
#include "../types.h"
#include "../Blob.h"
#include "../Descriptor.h"
#include "../NumericConverter.h"
#include "../CalendarConverter.h"
#include "../impl/ParameterSetter.h"
#include "RequestMessageFormat.h"
#include "../Row.h"
#include "../SmartPtrs.h"
#include "../Exception.h"
#include "../StructBinding.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
#include <boost/decimal.hpp>
#endif


///
/// fb-cpp request namespace.
///
namespace fbcpp::request
{
	///
	/// A message buffer of a Firebird request with typed accessors.
	///
	/// RequestMessage is created by Request from a RequestMessageFormat and keeps its buffer private. Values are read
	/// through getRow() (a Row over the message) and written with the same setter family used for Statement input
	/// parameters.
	///
	/// Setters use the same conversions as Statement. Assigning null (setNull or an empty optional) to a non-nullable
	/// field throws because non-nullable request fields do not have null-indicator storage.
	///
	class FB_CPP_EXPORT RequestMessage final : private impl::ParameterSetter<RequestMessage, true>
	{
		friend class impl::ParameterSetter<RequestMessage, true>;

	public:
		RequestMessage(RequestMessage&& o) noexcept
			: client{o.client},
			  descriptors{std::move(o.descriptors)},
			  message{std::move(o.message)},
			  statusWrapper{std::move(o.statusWrapper)},
			  numericConverter{std::move(o.numericConverter)},
			  calendarConverter{std::move(o.calendarConverter)},
			  row{std::make_unique<Row>(*client, descriptors, std::span<const std::byte>{message})}
		{
		}

		RequestMessage& operator=(RequestMessage&& o) noexcept
		{
			if (this != &o)
			{
				client = o.client;
				descriptors = std::move(o.descriptors);
				message = std::move(o.message);
				statusWrapper = std::move(o.statusWrapper);
				numericConverter = std::move(o.numericConverter);
				calendarConverter = std::move(o.calendarConverter);
				row = std::make_unique<Row>(*client, descriptors, std::span<const std::byte>{message});
			}

			return *this;
		}

		RequestMessage(const RequestMessage&) = delete;
		RequestMessage& operator=(const RequestMessage&) = delete;

	public:
		///
		/// Returns a Row view over the message, providing typed accessors for reading field values.
		///
		Row& getRow() noexcept
		{
			assert(row);
			return *row;
		}

		///
		/// Returns the format-derived descriptors of the message fields.
		///
		const std::vector<Descriptor>& getDescriptors() const noexcept
		{
			return descriptors;
		}

		///
		/// @name Parameter writing
		/// @{
		using impl::ParameterSetter<RequestMessage, true>::clearParameters;
		using impl::ParameterSetter<RequestMessage, true>::set;
		using impl::ParameterSetter<RequestMessage, true>::setBlobId;
		using impl::ParameterSetter<RequestMessage, true>::setBool;
		using impl::ParameterSetter<RequestMessage, true>::setBytes;
		using impl::ParameterSetter<RequestMessage, true>::setDate;
		using impl::ParameterSetter<RequestMessage, true>::setDouble;
		using impl::ParameterSetter<RequestMessage, true>::setFloat;
		using impl::ParameterSetter<RequestMessage, true>::setInt16;
		using impl::ParameterSetter<RequestMessage, true>::setInt32;
		using impl::ParameterSetter<RequestMessage, true>::setInt64;
		using impl::ParameterSetter<RequestMessage, true>::setNull;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueDate;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueDecFloat16;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueDecFloat34;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueInt128;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueTime;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueTimeTz;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueTimestamp;
		using impl::ParameterSetter<RequestMessage, true>::setOpaqueTimestampTz;
		using impl::ParameterSetter<RequestMessage, true>::setScaledInt16;
		using impl::ParameterSetter<RequestMessage, true>::setScaledInt32;
		using impl::ParameterSetter<RequestMessage, true>::setScaledInt64;
		using impl::ParameterSetter<RequestMessage, true>::setScaledOpaqueInt128;
		using impl::ParameterSetter<RequestMessage, true>::setString;
		using impl::ParameterSetter<RequestMessage, true>::setTime;
		using impl::ParameterSetter<RequestMessage, true>::setTimeTz;
		using impl::ParameterSetter<RequestMessage, true>::setTimestamp;
		using impl::ParameterSetter<RequestMessage, true>::setTimestampTz;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		using impl::ParameterSetter<RequestMessage, true>::setBoostDecFloat16;
		using impl::ParameterSetter<RequestMessage, true>::setBoostDecFloat34;
		using impl::ParameterSetter<RequestMessage, true>::setBoostInt128;
		using impl::ParameterSetter<RequestMessage, true>::setScaledBoostInt128;
#endif

#if FB_CPP_USE_BOOST_DECIMAL != 0
		using impl::ParameterSetter<RequestMessage, true>::setBoostDecimal128;
		using impl::ParameterSetter<RequestMessage, true>::setBoostDecimal32;
		using impl::ParameterSetter<RequestMessage, true>::setBoostDecimal64;
#endif

		///
		/// @}


	private:
		friend class Request;

		const std::vector<Descriptor>& getSetterDescriptors() const noexcept
		{
			return descriptors;
		}

		std::vector<std::byte>& getSetterMessage() noexcept
		{
			return message;
		}

		Client& getSetterClient() noexcept
		{
			return *client;
		}

		void validateSetter() const noexcept { }

		explicit RequestMessage(Client& c, const RequestMessageFormat& format)
			: client{&c},
			  descriptors{format.getDescriptors()},
			  message(format.getLength(), std::byte{0}),
			  statusWrapper{c},
			  numericConverter{c},
			  calendarConverter{c},
			  row{std::make_unique<Row>(c, descriptors, std::span<const std::byte>{message})}
		{
			clearParameters();
		}

	private:
		Client* client;
		std::vector<Descriptor> descriptors;
		std::vector<std::byte> message;
		impl::StatusWrapper statusWrapper;
		impl::NumericConverter numericConverter;
		impl::CalendarConverter calendarConverter;
		std::unique_ptr<Row> row;
	};
}  // namespace fbcpp::request


#endif  // FBCPP_REQUEST_MESSAGE_H
