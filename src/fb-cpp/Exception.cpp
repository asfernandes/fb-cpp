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

#include "Exception.h"
#include "Client.h"
#include <string>
#include <vector>
#include <cassert>

using namespace fbcpp;
using namespace fbcpp::impl;


void StatusWrapper::checkException(StatusWrapper* status)
{
	if (status->dirty && (status->getState() & fb::IStatus::STATE_ERRORS))
		throw DatabaseException{status->client, status->getErrors()};
}

void StatusWrapper::catchException(fb::IStatus* status) noexcept
{
	assert(false);
}


std::string DatabaseException::buildMessage(Client& client, const std::intptr_t* statusVector)
{
	constexpr char DEFAULT_MESSAGE[] = "Unknown database error";

	if (!statusVector)
		return DEFAULT_MESSAGE;

	const auto util = client.getUtil();

	const auto status = client.newStatus();
	status->setErrors(statusVector);

	constexpr unsigned MAX_BUFFER_SIZE = 32u * 1024u;
	unsigned bufferSize = 256u;
	std::string message;

	while (bufferSize <= MAX_BUFFER_SIZE)
	{
		std::string buffer(bufferSize, '\0');
		const auto written = util->formatStatus(buffer.data(), bufferSize, status.get());

		if (written < bufferSize && buffer[0] != '\0')
		{
			message = written == 0 ? std::string{buffer.c_str()} : std::string{buffer.data(), written};
			break;
		}

		if (bufferSize == MAX_BUFFER_SIZE)
		{
			message = buffer.c_str();
			break;
		}

		bufferSize = (bufferSize > MAX_BUFFER_SIZE / 2u) ? MAX_BUFFER_SIZE : bufferSize * 2u;
	}

	if (message.empty())
		message = DEFAULT_MESSAGE;

	return message;
}


std::vector<std::intptr_t> DatabaseException::copyErrorVector(const std::intptr_t* statusVector)
{
	std::vector<std::intptr_t> result;

	if (!statusVector)
		return result;

	const auto* p = statusVector;

	while (*p != isc_arg_end)
	{
		const auto argType = *p++;

		// clang-format off
		switch (argType)
		{
			case isc_arg_gds:
			case isc_arg_number:
				result.push_back(argType);
				result.push_back(*p++);
				break;

			case isc_arg_string:
			case isc_arg_interpreted:
			case isc_arg_sql_state:
				p++;  // skip string pointer
				break;

			case isc_arg_cstring:
				p += 2;  // skip length + string pointer
				break;

			default:
				p++;  // skip unknown arg value
				break;
		}
		// clang-format on
	}

	result.push_back(isc_arg_end);

	return result;
}


std::string DatabaseException::extractSqlState(const std::intptr_t* statusVector)
{
	if (!statusVector)
		return {};

	const auto* p = statusVector;

	while (*p != isc_arg_end)
	{
		const auto argType = *p++;

		if (argType == isc_arg_sql_state)
			return reinterpret_cast<const char*>(*p);

		if (argType == isc_arg_cstring)
			p += 2;
		else
			p++;
	}

	return {};
}
