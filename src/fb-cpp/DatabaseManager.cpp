/*
 * MIT License
 *
 * Copyright (c) 2026 Popa Adrian Marius
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

#include "DatabaseManager.h"
#include "Client.h"
#include <cassert>

using namespace fbcpp;
using namespace fbcpp::impl;


void DatabaseManager::setProperties(const DatabasePropertiesOptions& options)
{
	StatusWrapper statusWrapper{getClient()};
	auto builder =
		fbUnique(getClient().getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_START, nullptr, 0));
	builder->insertTag(&statusWrapper, isc_action_svc_properties);
	builder->insertString(&statusWrapper, isc_spb_dbname, options.getDatabase().c_str());

	if (const auto replicaMode = options.getReplicaMode())
	{
		std::uint8_t modeVal = 0;

		switch (*replicaMode)
		{
			case ReplicaMode::NONE:
				modeVal = isc_spb_prp_rm_none;
				break;
			case ReplicaMode::READ_ONLY:
				modeVal = isc_spb_prp_rm_readonly;
				break;
			case ReplicaMode::READ_WRITE:
				modeVal = isc_spb_prp_rm_readwrite;
				break;
			default:
				assert(false);
				break;
		}

		builder->insertBytes(&statusWrapper, isc_spb_prp_replica_mode, &modeVal, 1u);
	}

	if (const auto shutdownMode = options.getShutdownMode())
	{
		std::uint8_t stateVal = 0;

		switch (*shutdownMode)
		{
			case ShutdownMode::NORMAL:
				stateVal = isc_spb_prp_sm_normal;
				break;
			case ShutdownMode::MULTI:
				stateVal = isc_spb_prp_sm_multi;
				break;
			case ShutdownMode::SINGLE:
				stateVal = isc_spb_prp_sm_single;
				break;
			case ShutdownMode::FULL:
				stateVal = isc_spb_prp_sm_full;
				break;
			default:
				assert(false);
				break;
		}

		builder->insertBytes(&statusWrapper, isc_spb_prp_shutdown_mode, &stateVal, 1u);
	}

	if (const auto shutdownType = options.getShutdownType())
	{
		const auto timeout = options.getShutdownTimeout().value_or(0);

		switch (*shutdownType)
		{
			case ShutdownType::DENY_TRANSACTIONS:
				builder->insertInt(&statusWrapper, isc_spb_prp_deny_new_transactions, timeout);
				break;
			case ShutdownType::DENY_ATTACHMENTS:
				builder->insertInt(&statusWrapper, isc_spb_prp_deny_new_attachments, timeout);
				break;
			case ShutdownType::FORCED:
				builder->insertInt(&statusWrapper, isc_spb_prp_force_shutdown, timeout);
				break;
			default:
				assert(false);
				break;
		}
	}

	if (const auto online = options.getOnline())
	{
		if (*online)
		{
			std::uint8_t stateVal = isc_spb_prp_sm_normal;
			builder->insertBytes(&statusWrapper, isc_spb_prp_online_mode, &stateVal, 1u);
		}
	}

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	startAction(std::vector<std::uint8_t>(buffer, buffer + length));
	waitForCompletion();
}


void DatabaseManager::repair(const DatabaseRepairOptions& options)
{
	StatusWrapper statusWrapper{getClient()};
	auto builder =
		fbUnique(getClient().getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_START, nullptr, 0));

	builder->insertTag(&statusWrapper, isc_action_svc_repair);
	builder->insertString(&statusWrapper, isc_spb_dbname, options.getDatabase().c_str());

	int optionsVal = 0;
	if (options.getSweep())
		optionsVal |= isc_spb_rpr_sweep_db;
	if (options.getValidate())
		optionsVal |= isc_spb_rpr_validate_db;
	if (options.getMend())
		optionsVal |= isc_spb_rpr_mend_db;
	if (options.getIgnoreChecksum())
		optionsVal |= isc_spb_rpr_ignore_checksum;
	if (options.getKillShadows())
		optionsVal |= isc_spb_rpr_kill_shadows;
	if (options.getFull())
		optionsVal |= isc_spb_rpr_full;
	if (options.getCheckDb())
		optionsVal |= isc_spb_rpr_check_db;
	if (options.getIcu())
		optionsVal |= isc_spb_rpr_icu;
	if (options.getUpgradeDb())
		optionsVal |= isc_spb_rpr_upgrade_db;

	if (optionsVal != 0)
		builder->insertInt(&statusWrapper, isc_spb_options, optionsVal);

	if (const auto parallelWorkers = options.getParallelWorkers())
		builder->insertInt(&statusWrapper, isc_spb_rpr_par_workers, static_cast<int>(*parallelWorkers));

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	startAction(std::vector<std::uint8_t>(buffer, buffer + length));
	waitForCompletion(options.getVerboseOutput());
}
