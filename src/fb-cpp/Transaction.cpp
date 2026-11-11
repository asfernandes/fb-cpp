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
#include "firebird/impl/inf_pub.h"
#include <cassert>
#include <cstdint>

using namespace fbcpp;
using namespace fbcpp::impl;


static FbUniquePtr<fb::IXpbBuilder> buildTpb(
	fb::IMaster* master, StatusWrapper& statusWrapper, const TransactionOptions& options)
{
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

	if (const auto atSnapshotNumber = options.getAtSnapshotNumber())
	{
		tpbBuilder->insertBigInt(
			&statusWrapper, isc_tpb_at_snapshot_number, static_cast<std::int64_t>(atSnapshotNumber.value()));
	}

	if (options.getNoAutoUndo())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_no_auto_undo);

	if (options.getIgnoreLimbo())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_ignore_limbo);

	if (options.getRestartRequests())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_restart_requests);

	if (options.getAutoCommit())
		tpbBuilder->insertTag(&statusWrapper, isc_tpb_autocommit);

	return tpbBuilder;
}


Transaction::Transaction(Attachment& attachment, const TransactionOptions& options)
	: client{attachment.getClient()}
{
	assert(attachment.isValid());

	const auto master = client.getMaster();

	StatusWrapper statusWrapper{client};

	auto tpbBuilder = buildTpb(master, statusWrapper, options);
	const auto tpbBuffer = tpbBuilder->getBuffer(&statusWrapper);
	const auto tpbBufferLen = tpbBuilder->getBufferLength(&statusWrapper);

	handle.reset(attachment.getHandle()->startTransaction(&statusWrapper, tpbBufferLen, tpbBuffer));
}

Transaction::Transaction(Attachment& attachment, std::string_view setTransactionCmd)
	: client{attachment.getClient()}
{
	assert(attachment.isValid());

	StatusWrapper statusWrapper{client};

	handle.reset(
		attachment.getHandle()->execute(&statusWrapper, nullptr, static_cast<unsigned>(setTransactionCmd.length()),
			setTransactionCmd.data(), SQL_DIALECT_V6, nullptr, nullptr, nullptr, nullptr));
}

Transaction::Transaction(std::span<std::reference_wrapper<Attachment>> attachments, const TransactionOptions& options)
	: client{attachments[0].get().getClient()},
	  isMultiDatabase{true}
{
	assert(!attachments.empty());

	// Validate all attachments use the same Client
	for (const auto& attachment : attachments)
	{
		assert(attachment.get().isValid());

		if (&attachment.get().getClient() != &client)
			throw std::invalid_argument("All attachments must use the same Client for multi-database transactions");
	}

	const auto master = client.getMaster();

	StatusWrapper statusWrapper{client};

	auto tpbBuilder = buildTpb(master, statusWrapper, options);
	const auto tpbBuffer = tpbBuilder->getBuffer(&statusWrapper);
	const auto tpbBufferLen = tpbBuilder->getBufferLength(&statusWrapper);

	auto dtcInterface = master->getDtc();
	auto dtcStart = fbUnique(dtcInterface->startBuilder(&statusWrapper));

	// Add each attachment with the same TPB
	for (const auto& attachment : attachments)
		dtcStart->addWithTpb(&statusWrapper, attachment.get().getHandle().get(), tpbBufferLen, tpbBuffer);

	// Start the multi-database transaction, which disposes the IDtcStart instance
	handle.reset(dtcStart->start(&statusWrapper));
	dtcStart.release();
}

void Transaction::rollback()
{
	assert(isValid());
	assert(state == TransactionState::ACTIVE || state == TransactionState::PREPARED);

	StatusWrapper statusWrapper{client};

	handle->rollback(&statusWrapper);
	handle.release();
	state = TransactionState::ROLLED_BACK;
}

void Transaction::commit()
{
	assert(isValid());
	assert(state == TransactionState::ACTIVE || state == TransactionState::PREPARED);

	StatusWrapper statusWrapper{client};

	handle->commit(&statusWrapper);
	handle.release();
	state = TransactionState::COMMITTED;
}

void Transaction::commitRetaining()
{
	assert(isValid());
	assert(state == TransactionState::ACTIVE);

	StatusWrapper statusWrapper{client};

	handle->commitRetaining(&statusWrapper);
}

void Transaction::rollbackRetaining()
{
	assert(isValid());
	assert(state == TransactionState::ACTIVE);

	StatusWrapper statusWrapper{client};

	handle->rollbackRetaining(&statusWrapper);
}

std::uint64_t Transaction::getSnapshotNumber()
{
	assert(isValid());

	StatusWrapper statusWrapper{client};

	const std::uint8_t items[] = {fb_info_tra_snapshot_number, isc_info_end};
	std::uint8_t buffer[32]{};

	handle->getInfo(&statusWrapper, sizeof(items), items, sizeof(buffer), buffer);

	const auto* ptr = buffer;
	const auto* end = buffer + sizeof(buffer);

	while (ptr < end)
	{
		const auto item = *ptr++;

		if (item == isc_info_end)
			break;

		if (item == isc_info_truncated)
			throw FbCppException("Transaction::getSnapshotNumber truncated response");

		if (item == isc_info_error)
			throw FbCppException("Transaction::getSnapshotNumber error response");

		if (ptr + 2 > end)
			throw FbCppException("Transaction::getSnapshotNumber malformed response");

		const auto itemLength = static_cast<std::uint16_t>((ptr[0]) | (ptr[1] << 8));
		ptr += 2;

		if (ptr + itemLength > end || itemLength > sizeof(std::uint64_t))
			throw FbCppException("Transaction::getSnapshotNumber invalid length");

		if (item == fb_info_tra_snapshot_number)
		{
			std::uint64_t result = 0;

			for (std::uint16_t i = 0; i < itemLength; ++i)
				result |= static_cast<std::uint64_t>(ptr[i]) << (8u * i);

			return result;
		}

		ptr += itemLength;
	}

	throw FbCppException("Transaction::getSnapshotNumber not supported by the server (Firebird 4.0 or later required)");
}

void Transaction::prepare()
{
	prepare(std::span<const std::uint8_t>{});
}

void Transaction::prepare(std::span<const std::uint8_t> message)
{
	assert(isValid());
	assert(state == TransactionState::ACTIVE);

	StatusWrapper statusWrapper{client};

	handle->prepare(&statusWrapper, static_cast<unsigned>(message.size()), message.data());
	state = TransactionState::PREPARED;
}

void Transaction::prepare(std::string_view message)
{
	const auto messageBytes = reinterpret_cast<const std::uint8_t*>(message.data());
	prepare(std::span<const std::uint8_t>{messageBytes, message.size()});
}
