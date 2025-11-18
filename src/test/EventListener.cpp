/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to do so, subject to the following conditions:
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

#include "TestUtil.h"
#include "fb-cpp/EventListener.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>


using namespace std::chrono_literals;


BOOST_AUTO_TEST_SUITE(EventListenerSuite)

BOOST_AUTO_TEST_CASE(receivesSingleEvent)
{
	const auto database = getTempFile("EventListener-receivesSingleEvent.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	std::mutex mutex;
	std::condition_variable condition;
	std::vector<EventCount> receivedCounts;
	bool done = false;

	const auto listenerCallback = [&](const std::vector<EventCount>& counts)
	{
		std::lock_guard mutexGuard{mutex};
		receivedCounts = counts;
		done = true;
		condition.notify_one();
	};

	EventListener listener{attachment, {"EVENT_A"}, listenerCallback};

	{  // scope
		Transaction transaction{attachment};
		Statement statement{attachment, transaction, "execute block as begin post_event 'EVENT_A'; end"};
		BOOST_CHECK(statement.execute(transaction));
		transaction.commit();
	}

	{  // scope
		std::unique_lock mutexGuard{mutex};
		BOOST_REQUIRE(condition.wait_for(mutexGuard, 5s, [&] { return done; }));
	}

	BOOST_REQUIRE_EQUAL(receivedCounts.size(), 1u);
	BOOST_CHECK_EQUAL(receivedCounts.front().name, "EVENT_A");
	BOOST_CHECK_EQUAL(receivedCounts.front().count, 1u);
}

BOOST_AUTO_TEST_CASE(aggregatesMultipleEvents)
{
	const auto database = getTempFile("EventListener-aggregatesMultipleEvents.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		// Post some events before the listener is created.
		Transaction transaction{attachment};
		Statement statement{attachment, transaction, R"""(
			execute block
			as
			begin
			    post_event 'EVENT_ALPHA';
			    post_event 'EVENT_ALPHA';
			    post_event 'EVENT_ALPHA';
			    post_event 'EVENT_ALPHA';
			end
		)"""};
		BOOST_CHECK(statement.execute(transaction));
		transaction.commit();
	}

	std::mutex mutex;
	std::condition_variable condition;
	std::vector<std::vector<EventCount>> notifications;

	const auto listenerCallback = [&](const std::vector<EventCount>& counts)
	{
		std::lock_guard mutexGuard{mutex};
		notifications.emplace_back(counts);
		condition.notify_all();
	};

	EventListener listener{attachment, {"EVENT_ALPHA", "EVENT_BETA"}, listenerCallback};

	{  // scope
		Transaction transaction{attachment};
		Statement statement{attachment, transaction, R"""(
			execute block
			as
			begin
			    post_event 'EVENT_ALPHA';
			    post_event 'EVENT_BETA';
			    post_event 'EVENT_ALPHA';
			end
		)"""};
		BOOST_CHECK(statement.execute(transaction));
		transaction.commit();
	}

	std::unique_lock mutexGuard{mutex};
	BOOST_REQUIRE(condition.wait_for(mutexGuard, 5s, [&] { return !notifications.empty(); }));
	auto captured = notifications.front();
	mutexGuard.unlock();

	BOOST_REQUIRE_EQUAL(captured.size(), 2u);
	BOOST_CHECK_EQUAL(captured[0].name, "EVENT_ALPHA");
	BOOST_CHECK_EQUAL(captured[0].count, 2u);
	BOOST_CHECK_EQUAL(captured[1].name, "EVENT_BETA");
	BOOST_CHECK_EQUAL(captured[1].count, 1u);

	listener.stop();
}

BOOST_AUTO_TEST_CASE(stopsReceivingEventsAfterStop)
{
	const auto database = getTempFile("EventListener-stopsReceivingEventsAfterStop.fdb");
	Attachment attachment{CLIENT, database, AttachmentOptions().setCreateDatabase(true).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	std::mutex mutex;
	std::condition_variable condition;
	std::vector<EventCount> lastNotification;
	unsigned callbackInvocations = 0;

	const auto listenerCallback = [&](const std::vector<EventCount>& counts)
	{
		std::lock_guard mutexGuard{mutex};
		lastNotification = counts;
		++callbackInvocations;
		condition.notify_all();
	};

	EventListener listener{attachment, {"EVENT_STOP"}, listenerCallback};
	BOOST_CHECK(listener.isListening());

	{  // scope
		Transaction transaction{attachment};
		Statement statement{attachment, transaction, "execute block as begin post_event 'EVENT_STOP'; end"};
		BOOST_CHECK(statement.execute(transaction));
		transaction.commit();
	}

	std::vector<EventCount> receivedBeforeStop;

	{  // scope
		std::unique_lock mutexGuard{mutex};
		BOOST_REQUIRE(condition.wait_for(mutexGuard, 5s, [&] { return callbackInvocations > 0; }));
		receivedBeforeStop = lastNotification;
	}

	BOOST_REQUIRE_EQUAL(receivedBeforeStop.size(), 1u);
	BOOST_CHECK_EQUAL(receivedBeforeStop.front().name, "EVENT_STOP");
	BOOST_CHECK_EQUAL(receivedBeforeStop.front().count, 1u);

	listener.stop();
	BOOST_CHECK(!listener.isListening());

	{  // scope
		Transaction transaction{attachment};
		Statement statement{attachment, transaction, "execute block as begin post_event 'EVENT_STOP'; end"};
		BOOST_CHECK(statement.execute(transaction));
		transaction.commit();
	}

	{  // scope
		std::unique_lock mutexGuard{mutex};
		const auto receivedAfterStop = condition.wait_for(mutexGuard, 1s, [&] { return callbackInvocations > 1; });
		BOOST_CHECK(!receivedAfterStop);
		BOOST_REQUIRE_EQUAL(callbackInvocations, 1u);
	}
}

BOOST_AUTO_TEST_SUITE_END()
