#include <chrono>
#include <string>
#include <thread>

#include <boost/test/unit_test.hpp>
#include <magic_enum/magic_enum.hpp>

#include "developer/async_operation_tracker.h"

using namespace AqualinkAutomate::Developer;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(AsyncOperationTrackerTests)

BOOST_AUTO_TEST_CASE(Test_StartComplete_Lifecycle)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto pending_before = tracker.PendingCount();

	auto id = tracker.OnStarted(AsyncOperationType::SerialRead, "TestRead");
	BOOST_TEST(id > 0u);
	BOOST_TEST(tracker.PendingCount() == pending_before + 1);

	tracker.OnCompleted(id);
	BOOST_TEST(tracker.PendingCount() == pending_before);
}

BOOST_AUTO_TEST_CASE(Test_ScopedOp_AutoComplete)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto pending_before = tracker.PendingCount();

	{
		ScopedOp op(AsyncOperationType::TimerWait, "ScopedTimerTest");
		BOOST_TEST(op.Id() > 0u);
		BOOST_TEST(tracker.PendingCount() == pending_before + 1);
	}

	// After scope exit, should be auto-completed
	BOOST_TEST(tracker.PendingCount() == pending_before);
}

BOOST_AUTO_TEST_CASE(Test_ScopedOp_ExplicitComplete)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto pending_before = tracker.PendingCount();

	{
		ScopedOp op(AsyncOperationType::SerialWrite, "ExplicitCompleteTest");
		BOOST_TEST(tracker.PendingCount() == pending_before + 1);

		op.Complete(true, false); // cancelled
		BOOST_TEST(tracker.PendingCount() == pending_before);
	}

	// Destructor should not double-complete
	BOOST_TEST(tracker.PendingCount() == pending_before);
}

BOOST_AUTO_TEST_CASE(Test_GetStuck)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto id = tracker.OnStarted(AsyncOperationType::SignalAwait, "StuckTest");

	std::this_thread::sleep_for(50ms);

	auto stuck = tracker.GetStuck(10ms);
	bool found = std::any_of(stuck.begin(), stuck.end(),
		[id](const AsyncOperationInfo& info) { return info.id == id; });
	BOOST_TEST(found);

	auto not_stuck = tracker.GetStuck(10s);
	bool not_found = std::none_of(not_stuck.begin(), not_stuck.end(),
		[id](const AsyncOperationInfo& info) { return info.id == id; });
	BOOST_TEST(not_found);

	tracker.OnCompleted(id);
}

BOOST_AUTO_TEST_CASE(Test_PendingCountByType)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto serial_before = tracker.PendingCountByType(AsyncOperationType::SerialRead);
	auto mqtt_before = tracker.PendingCountByType(AsyncOperationType::MqttPublish);

	auto id1 = tracker.OnStarted(AsyncOperationType::SerialRead, "TypeFilterTest1");
	auto id2 = tracker.OnStarted(AsyncOperationType::MqttPublish, "TypeFilterTest2");

	BOOST_TEST(tracker.PendingCountByType(AsyncOperationType::SerialRead) == serial_before + 1);
	BOOST_TEST(tracker.PendingCountByType(AsyncOperationType::MqttPublish) == mqtt_before + 1);

	tracker.OnCompleted(id1);
	BOOST_TEST(tracker.PendingCountByType(AsyncOperationType::SerialRead) == serial_before);
	BOOST_TEST(tracker.PendingCountByType(AsyncOperationType::MqttPublish) == mqtt_before + 1);

	tracker.OnCompleted(id2);
}

BOOST_AUTO_TEST_CASE(Test_CancelVsErrorVsNormal)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto id_normal = tracker.OnStarted(AsyncOperationType::Other, "NormalComplete");
	auto id_cancel = tracker.OnStarted(AsyncOperationType::Other, "CancelComplete");
	auto id_error = tracker.OnStarted(AsyncOperationType::Other, "ErrorComplete");

	tracker.OnCompleted(id_normal, false, false);
	tracker.OnCompleted(id_cancel, true, false);
	tracker.OnCompleted(id_error, false, true);

	// All should be completed (not pending)
	auto pending = tracker.GetPending();
	bool any_found = std::any_of(pending.begin(), pending.end(),
		[&](const AsyncOperationInfo& info)
		{
			return info.id == id_normal || info.id == id_cancel || info.id == id_error;
		});
	BOOST_TEST(!any_found);
}

BOOST_AUTO_TEST_CASE(Test_ZeroId_Complete_Safe)
{
	auto& tracker = AsyncOperationTracker::Instance();

	// Completing id 0 should be safe (no-op)
	tracker.OnCompleted(0);
	BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(Test_EnumName)
{
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::SerialRead)) == "SerialRead");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::SerialWrite)) == "SerialWrite");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::TimerWait)) == "TimerWait");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::SignalAwait)) == "SignalAwait");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::WebSocketRead)) == "WebSocketRead");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::WebSocketWrite)) == "WebSocketWrite");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::HttpAccept)) == "HttpAccept");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::MqttPublish)) == "MqttPublish");
	BOOST_TEST(std::string(magic_enum::enum_name(AsyncOperationType::Other)) == "Other");
}

BOOST_AUTO_TEST_CASE(Test_GetPending_ReturnsOnlyIncomplete)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto id1 = tracker.OnStarted(AsyncOperationType::SerialRead, "PendingTest1");
	auto id2 = tracker.OnStarted(AsyncOperationType::SerialWrite, "PendingTest2");
	auto id3 = tracker.OnStarted(AsyncOperationType::TimerWait, "PendingTest3");

	tracker.OnCompleted(id2);

	auto pending = tracker.GetPending();

	bool found_id1 = std::any_of(pending.begin(), pending.end(),
		[id1](const AsyncOperationInfo& info) { return info.id == id1; });
	bool found_id2 = std::any_of(pending.begin(), pending.end(),
		[id2](const AsyncOperationInfo& info) { return info.id == id2; });
	bool found_id3 = std::any_of(pending.begin(), pending.end(),
		[id3](const AsyncOperationInfo& info) { return info.id == id3; });

	BOOST_TEST(found_id1);
	BOOST_TEST(!found_id2);
	BOOST_TEST(found_id3);

	tracker.OnCompleted(id1);
	tracker.OnCompleted(id3);
}

BOOST_AUTO_TEST_CASE(Test_DoubleComplete_Idempotent)
{
	auto& tracker = AsyncOperationTracker::Instance();

	auto pending_before = tracker.PendingCount();

	auto id = tracker.OnStarted(AsyncOperationType::Other, "DoubleCompleteTest");
	BOOST_TEST(tracker.PendingCount() == pending_before + 1);

	tracker.OnCompleted(id);
	BOOST_TEST(tracker.PendingCount() == pending_before);

	// Second completion should be a no-op
	tracker.OnCompleted(id);
	BOOST_TEST(tracker.PendingCount() == pending_before);
}

BOOST_AUTO_TEST_CASE(Test_CompleteNonexistentId_Safe)
{
	auto& tracker = AsyncOperationTracker::Instance();

	// Completing a nonexistent high ID should be safe
	tracker.OnCompleted(999999999);
	BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()
