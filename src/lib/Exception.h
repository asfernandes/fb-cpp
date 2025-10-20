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

#ifndef FBCPP_EXCEPTION_H
#define FBCPP_EXCEPTION_H

#include "fb-api.h"
#include <stdexcept>
#include <string>
#include <cstdint>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Client;
}

namespace fbcpp::impl
{
	class StatusWrapper : public fb::IStatusImpl<StatusWrapper, StatusWrapper>
	{
	public:
		explicit StatusWrapper(Client& client, IStatus* status)
			: client{client},
			  status{status}
		{
		}

	public:
		static void checkException(StatusWrapper* status);

		static void catchException(IStatus* status) noexcept;

		static void clearException(StatusWrapper* status) noexcept
		{
			status->clearException();
		}

		void clearException()
		{
			if (dirty)
			{
				dirty = false;
				status->init();
			}
		}

		bool isDirty() const noexcept
		{
			return dirty;
		}

		bool hasData() const noexcept
		{
			return getState() & IStatus::STATE_ERRORS;
		}

		bool isEmpty() const noexcept
		{
			return !hasData();
		}

		static void setVersionError(
			IStatus* status, const char* interfaceName, uintptr_t currentVersion, uintptr_t expectedVersion) noexcept
		{
			// clang-format off
			const intptr_t codes[] = {
				isc_arg_gds, isc_interface_version_too_old,
				isc_arg_number, (intptr_t) expectedVersion,
				isc_arg_number, (intptr_t) currentVersion,
				isc_arg_string, (intptr_t) interfaceName,
				isc_arg_end,
			};
			// clang-format on

			status->setErrors(codes);
		}

	public:
		void dispose() noexcept override
		{
			// Disposes only the delegated status. Let the user destroy this instance.
			status->dispose();
			status = nullptr;
		}

		void init() noexcept override
		{
			clearException();
		}

		unsigned getState() const noexcept override
		{
			return dirty ? status->getState() : 0;
		}

		void setErrors2(unsigned length, const intptr_t* value) noexcept override
		{
			dirty = true;
			status->setErrors2(length, value);
		}

		void setWarnings2(unsigned length, const intptr_t* value) noexcept override
		{
			dirty = true;
			status->setWarnings2(length, value);
		}

		void setErrors(const intptr_t* value) noexcept override
		{
			dirty = true;
			status->setErrors(value);
		}

		void setWarnings(const intptr_t* value) noexcept override
		{
			dirty = true;
			status->setWarnings(value);
		}

		const intptr_t* getErrors() const noexcept override
		{
			return dirty ? status->getErrors() : cleanStatus();
		}

		const intptr_t* getWarnings() const noexcept override
		{
			return dirty ? status->getWarnings() : cleanStatus();
		}

		IStatus* clone() const noexcept override
		{
			return status->clone();
		}

	protected:
		Client& client;
		IStatus* status;
		bool dirty = false;

		static const intptr_t* cleanStatus() noexcept
		{
			static intptr_t clean[3] = {1, 0, 0};
			return clean;
		}
	};
}  // namespace fbcpp::impl


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class FbCppException : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;

		explicit FbCppException(const std::string& message)
			: std::runtime_error{message}
		{
		}
	};

	class DatabaseException final : public FbCppException
	{
	public:
		using FbCppException::FbCppException;

		explicit DatabaseException(Client& client, const std::intptr_t* status)
			: FbCppException{buildMessage(client, status)}
		{
		}

	private:
		static std::string buildMessage(Client& client, const std::intptr_t* status);
	};
}  // namespace fbcpp


#endif  // FBCPP_EXCEPTION_H
