#include <chrono>
#include <set>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "developer/signal_connection_tracker.h"

using namespace AqualinkAutomate::Developer;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(SignalConnectionTrackerTests)

BOOST_AUTO_TEST_CASE(Test_ConnectDisconnect_Lifecycle)
{
	auto& tracker = SignalConnectionTracker::Instance();

	auto active_before = tracker.ActiveCount();

	auto id = tracker.OnConnected("TestSignal");
	BOOST_TEST(id > 0u);
	BOOST_TEST(tracker.ActiveCount() == active_before + 1);

	tracker.OnDisconnected(id);
	BOOST_TEST(tracker.ActiveCount() == active_before);
}

BOOST_AUTO_TEST_CASE(Test_GetOlderThan)
{
	auto& tracker = SignalConnectionTracker::Instance();

	auto id = tracker.OnConnected("OldSignal");

	std::this_thread::sleep_for(50ms);

	auto old_connections = tracker.GetOlderThan(10ms);
	bool found = std::any_of(old_connections.begin(), old_connections.end(),
		[id](const SignalConnectionInfo& info) { return info.id == id; });
	BOOST_TEST(found);

	auto not_old = tracker.GetOlderThan(10s);
	bool not_found = std::none_of(not_old.begin(), not_old.end(),
		[id](const SignalConnectionInfo& info) { return info.id == id; });
	BOOST_TEST(not_found);

	tracker.OnDisconnected(id);
}

BOOST_AUTO_TEST_CASE(Test_DoubleDisconnect_Idempotent)
{
	auto& tracker = SignalConnectionTracker::Instance();

	auto active_before = tracker.ActiveCount();

	auto id = tracker.OnConnected("DoubleDisconnectSignal");
	BOOST_TEST(tracker.ActiveCount() == active_before + 1);

	tracker.OnDisconnected(id);
	BOOST_TEST(tracker.ActiveCount() == active_before);

	// Second disconnect should be a no-op
	tracker.OnDisconnected(id);
	BOOST_TEST(tracker.ActiveCount() == active_before);
}

BOOST_AUTO_TEST_CASE(Test_IdUniqueness)
{
	auto& tracker = SignalConnectionTracker::Instance();

	std::set<uint64_t> ids;
	constexpr int COUNT = 100;

	for (int i = 0; i < COUNT; ++i)
	{
		auto id = tracker.OnConnected("UniquenessTest");
		BOOST_TEST(ids.count(id) == 0u);
		ids.insert(id);
		tracker.OnDisconnected(id);
	}

	BOOST_TEST(ids.size() == COUNT);
}

BOOST_AUTO_TEST_CASE(Test_ZeroId_Disconnect_Safe)
{
	auto& tracker = SignalConnectionTracker::Instance();

	// Disconnecting id 0 should be safe (no-op)
	tracker.OnDisconnected(0);
	BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(Test_GetActive_ReturnsOnlyNonDisconnected)
{
	auto& tracker = SignalConnectionTracker::Instance();

	auto id1 = tracker.OnConnected("ActiveTest1");
	auto id2 = tracker.OnConnected("ActiveTest2");
	auto id3 = tracker.OnConnected("ActiveTest3");

	tracker.OnDisconnected(id2);

	auto active = tracker.GetActive();

	bool found_id1 = std::any_of(active.begin(), active.end(),
		[id1](const SignalConnectionInfo& info) { return info.id == id1; });
	bool found_id2 = std::any_of(active.begin(), active.end(),
		[id2](const SignalConnectionInfo& info) { return info.id == id2; });
	bool found_id3 = std::any_of(active.begin(), active.end(),
		[id3](const SignalConnectionInfo& info) { return info.id == id3; });

	BOOST_TEST(found_id1);
	BOOST_TEST(!found_id2);
	BOOST_TEST(found_id3);

	tracker.OnDisconnected(id1);
	tracker.OnDisconnected(id3);
}

BOOST_AUTO_TEST_SUITE_END()
