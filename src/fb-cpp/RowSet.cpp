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

#include "RowSet.h"
#include "Attachment.h"
#include "Client.h"
#include "Statement.h"
#include <algorithm>

using namespace fbcpp;
using namespace fbcpp::impl;


RowSet::RowSet(Statement& statement, unsigned maxRows)
	: RowSet{statement, maxRows, false}
{
}

RowSet::RowSet(Statement& statement, unsigned maxRows, bool includeCurrentRow)
	: client{&statement.getAttachment().getClient()},
	  statusWrapper{statement.getAttachment().getClient()},
	  numericConverter{statement.getAttachment().getClient()},
	  calendarConverter{statement.getAttachment().getClient()}
{
	assert(statement.isValid());
	assert(includeCurrentRow || statement.getResultSetHandle());

	descriptors = statement.getOutputDescriptors();

	auto outMetadata = statement.getOutputMetadata();
	messageLength = outMetadata->getMessageLength(&statusWrapper);

	buffer.resize(static_cast<std::size_t>(maxRows) * messageLength);

	auto resultSet = statement.getResultSetHandle();
	auto* dest = buffer.data();

	if (includeCurrentRow && maxRows > 0u)
	{
		auto& currentRow = statement.getOutputMessage();
		assert(currentRow.size() == messageLength);
		std::copy(currentRow.begin(), currentRow.end(), dest);
		dest += messageLength;
		++count;
	}

	if (!resultSet)
	{
		buffer.resize(static_cast<std::size_t>(dest - buffer.data()));
		return;
	}

	for (unsigned i = count; i < maxRows; ++i)
	{
		if (resultSet->fetchNext(&statusWrapper, dest) != fb::IStatus::RESULT_OK)
			break;

		dest += messageLength;
		++count;
	}

	buffer.resize(static_cast<std::size_t>(dest - buffer.data()));
}
