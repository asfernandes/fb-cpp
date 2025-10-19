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

#include "Transaction.h"
#include "Attachment.h"
#include "Client.h"
#include "Exception.h"
#include <cassert>

using namespace fbcpp;
using namespace fbcpp::impl;


Transaction::Transaction(Attachment& attachment, const TransactionOptions& options)
	: client{attachment.getClient()}
{
	assert(attachment.isValid());

	const auto master = client.getMaster();

	const auto status = client.newStatus();
	StatusWrapper statusWrapper{client, status.get()};

	auto tpbBuilder = fbUnique(master->getUtilInterface()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::TPB,
		reinterpret_cast<const std::uint8_t*>(options.getTpb().data()),
		static_cast<unsigned>(options.getTpb().size())));

	if (const auto accessMode = options.getAccessMode())
	{
		switch (accessMode.value())
		{
			case TransactionAccessMode::READ_ONLY:
				tpbBuilder->insertTag(&statusWrapper, isc_tpb_read);
				break;

			case TransactionAccessMode::READ_WRITE:
				tpbBuilder->insertTag(&statusWrapper, isc_tpb_write);
				break;

			default:
				assert(false);
				break;
		}
	}

	if (const auto waitMode = options.getWaitMode())
	{
		switch (waitMode.value())
		{
			case TransactionWaitMode::NO_WAIT:
				tpbBuilder->insertTag(&statusWrapper, isc_tpb_nowait);
				break;

			case TransactionWaitMode::WAIT:
				tpbBuilder->insertTag(&statusWrapper, isc_tpb_wait);
				break;

			default:
				assert(false);
				break;
		}
	}

	if (const auto isolationLevel = options.getIsolationLevel())
	{
		switch (isolationLevel.value())
		{
			case TransactionIsolationLevel::CONSISTENCY:
				tpbBuilder->insertTag(&statusWrapper, isc_tpb_consistency);
				break;

			case TransactionIsolationLevel::SNAPSHOT:
				tpbBuilder->insertTag(&statusWrapper, isc_tpb_concurrency);
				break;

			case TransactionIsolationLevel::READ_COMMITTED:
				tpbBuilder->insertTag(&statusWrapper, isc_tpb_read_committed);

				if (const auto readCommittedMode = options.getReadCommittedMode())
				{
					switch (readCommittedMode.value())
					{
						case TransactionReadCommittedMode::NO_RECORD_VERSION:
							tpbBuilder->insertTag(&statusWrapper, isc_tpb_no_rec_version);
							break;

						case TransactionReadCommittedMode::RECORD_VERSION:
							tpbBuilder->insertTag(&statusWrapper, isc_tpb_rec_version);
							break;

						default:
							assert(false);
							break;
					}
				}

				break;

			default:
				assert(false);
				break;
		}
	}

	if (options.getNoAutoUndo())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_no_auto_undo);

	if (options.getIgnoreLimbo())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_ignore_limbo);

	if (options.getRestartRequests())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_restart_requests);

	if (options.getAutoCommit())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_autocommit);

	const auto tpbBuffer = tpbBuilder->getBuffer(&statusWrapper);
	const auto tpbBufferLen = tpbBuilder->getBufferLength(&statusWrapper);

	handle.reset(attachment.getHandle()->startTransaction(&statusWrapper, tpbBufferLen, tpbBuffer));
}

Transaction::Transaction(Attachment& attachment, std::string_view setTransactionCmd)
	: client{attachment.getClient()}
{
	assert(attachment.isValid());

	const auto status = client.newStatus();
	StatusWrapper statusWrapper{client, status.get()};

	handle.reset(
		attachment.getHandle()->execute(&statusWrapper, nullptr, static_cast<unsigned>(setTransactionCmd.length()),
			setTransactionCmd.data(), SQL_DIALECT_V6, nullptr, nullptr, nullptr, nullptr));
}

void Transaction::rollback()
{
	assert(isValid());

	const auto status = client.newStatus();
	StatusWrapper statusWrapper{client, status.get()};

	handle->rollback(&statusWrapper);
	handle.reset();
}


void Transaction::commit()
{
	assert(isValid());

	const auto status = client.newStatus();
	StatusWrapper statusWrapper{client, status.get()};

	handle->commit(&statusWrapper);
	handle.reset();
}

void Transaction::commitRetaining()
{
	assert(isValid());

	const auto status = client.newStatus();
	StatusWrapper statusWrapper{client, status.get()};

	handle->commitRetaining(&statusWrapper);
}

void Transaction::rollbackRetaining()
{
	assert(isValid());

	const auto status = client.newStatus();
	StatusWrapper statusWrapper{client, status.get()};

	handle->rollbackRetaining(&statusWrapper);
}
