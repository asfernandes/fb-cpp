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

#include "Attachment.h"
#include "Client.h"
#include "Exception.h"
#include "RowSet.h"
#include "Statement.h"
#include "Transaction.h"

using namespace fbcpp;
using namespace fbcpp::impl;


Attachment::Attachment(Client& client, const std::string& uri, const AttachmentOptions& options)
	: client{&client}
{
	const auto master = client.getMaster();

	StatusWrapper statusWrapper{client};

	auto dpbBuilder = fbUnique(client.getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::DPB,
		reinterpret_cast<const std::uint8_t*>(options.getDpb().data()),
		static_cast<unsigned>(options.getDpb().size())));

	if (const auto connectionCharSet = options.getConnectionCharSet())
		dpbBuilder->insertString(&statusWrapper, isc_dpb_lc_ctype, connectionCharSet->c_str());

	if (const auto userName = options.getUserName())
		dpbBuilder->insertString(&statusWrapper, isc_dpb_user_name, userName->c_str());

	if (const auto password = options.getPassword())
		dpbBuilder->insertString(&statusWrapper, isc_dpb_password, password->c_str());

	if (const auto role = options.getRole())
		dpbBuilder->insertString(&statusWrapper, isc_dpb_sql_role_name, role->c_str());

	if (const auto sqlDialect = options.getSqlDialect())
		dpbBuilder->insertInt(&statusWrapper, isc_dpb_sql_dialect, static_cast<int>(*sqlDialect));

	if (const auto forcedWrites = options.getForcedWrites())
		dpbBuilder->insertInt(&statusWrapper, isc_dpb_force_write, *forcedWrites ? 1 : 0);

	auto dispatcher = fbRef(master->getDispatcher());
	const auto dpbBuffer = dpbBuilder->getBuffer(&statusWrapper);
	const auto dpbBufferLen = dpbBuilder->getBufferLength(&statusWrapper);

	if (options.getCreateDatabase())
		handle.reset(dispatcher->createDatabase(&statusWrapper, uri.c_str(), dpbBufferLen, dpbBuffer));
	else
		handle.reset(dispatcher->attachDatabase(&statusWrapper, uri.c_str(), dpbBufferLen, dpbBuffer));
}

void Attachment::disconnectOrDrop(bool drop)
{
	assert(isValid());

	StatusWrapper statusWrapper{*client};

	if (drop)
		handle->dropDatabase(&statusWrapper);
	else
		handle->detach(&statusWrapper);

	handle.reset();
}


void Attachment::disconnect()
{
	disconnectOrDrop(false);
}

void Attachment::dropDatabase()
{
	disconnectOrDrop(true);
}

void Attachment::ping()
{
	assert(isValid());

	StatusWrapper statusWrapper{*client};

	handle->ping(&statusWrapper);
}

void Attachment::resetSession()
{
	assert(isValid());

	StatusWrapper statusWrapper{*client};

	handle->execute(
		&statusWrapper, nullptr, 0, "alter session reset", SQL_DIALECT_V6, nullptr, nullptr, nullptr, nullptr);
}

bool Attachment::execute(Transaction& transaction, std::string_view sql, const StatementOptions& options)
{
	Statement statement{*this, transaction, sql, options};
	return statement.execute(transaction);
}

RowSet Attachment::queryRowSet(
	Transaction& transaction, std::string_view sql, unsigned maxRows, const StatementOptions& options)
{
	Statement statement{*this, transaction, sql, options};
	return queryPreparedRowSet(statement, transaction, maxRows);
}

RowSet Attachment::queryPreparedRowSet(Statement& statement, Transaction& transaction, unsigned maxRows)
{
	switch (statement.getType())
	{
		case StatementType::SELECT:
		case StatementType::SELECT_FOR_UPDATE:
			break;

		case StatementType::EXEC_PROCEDURE:
			if (!statement.getOutputDescriptors().empty())
				break;

			throw FbCppException("Cannot use procedure without output columns with Attachment::queryRowSet");

		default:
			throw FbCppException("Cannot use non-query SQL with Attachment::queryRowSet");
	}

	const auto hasRow = statement.execute(transaction);
	const auto effectiveMaxRows = statement.getType() == StatementType::EXEC_PROCEDURE ? 1u : maxRows;
	return RowSet{statement, hasRow ? effectiveMaxRows : 0u, hasRow};
}
