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


void DatabaseManager::execute(const DatabaseManagerOptions& options)
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

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	startAction(std::vector<std::uint8_t>(buffer, buffer + length));
	waitForCompletion();
}
