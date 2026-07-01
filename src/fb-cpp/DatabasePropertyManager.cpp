/*
 * MIT License
 *
 * Copyright (c) 2026 Adriano dos Santos Fernandes
 * ...
 */

#include "DatabasePropertyManager.h"
#include "Client.h"

using namespace fbcpp;
using namespace fbcpp::impl;

void DatabasePropertyManager::shutdown(const ShutdownOptions& options)
{
	StatusWrapper statusWrapper{getClient()};
	auto builder =
		fbUnique(getClient().getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_START, nullptr, 0));

	builder->insertTag(&statusWrapper, isc_action_svc_properties);
	builder->insertString(&statusWrapper, isc_spb_dbname, options.getDatabase().c_str());

	switch (options.getMode())
	{
		case ShutdownMode::DENY_TRANSACTIONS:
			builder->insertInt(&statusWrapper, isc_spb_prp_deny_new_transactions, options.getTimeout());
			break;
		case ShutdownMode::DENY_ATTACHMENTS:
			builder->insertInt(&statusWrapper, isc_spb_prp_deny_new_attachments, options.getTimeout());
			break;
		case ShutdownMode::FORCED:
		default:
			builder->insertInt(&statusWrapper, isc_spb_prp_force_shutdown, options.getTimeout());
			break;
	}

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	startAction(std::vector<std::uint8_t>(buffer, buffer + length));
	waitForCompletion();
}

void DatabasePropertyManager::startup(const std::string& database)
{
	StatusWrapper statusWrapper{getClient()};
	auto builder =
		fbUnique(getClient().getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_START, nullptr, 0));

	builder->insertTag(&statusWrapper, isc_action_svc_properties);
	builder->insertString(&statusWrapper, isc_spb_dbname, database.c_str());
	builder->insertInt(&statusWrapper, isc_spb_options, isc_spb_prp_db_online);

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	startAction(std::vector<std::uint8_t>(buffer, buffer + length));
	waitForCompletion();
}
