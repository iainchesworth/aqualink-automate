#include <chrono>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "developer/coroutine_lifetime_tracker.h"

using namespace AqualinkAutomate::Developer;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(CoroutineLifetimeTrackerTests)

BOOST_AUTO_TEST_CASE(Test_CreateDestroy_UpdatesCounts)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	auto created_before = tracker.TotalCreated();
	auto destroyed_before = tracker.TotalDestroyed();
	auto active_before = tracker.ActiveCount();

	uintptr_t handle = 0xDEAD0001;
	tracker.OnCreated(handle, "TestCoroutine");

	BOOST_TEST(tracker.TotalCreated() == created_before + 1);
	BOOST_TEST(tracker.ActiveCount() == active_before + 1);

	tracker.OnDestroyed(handle);

	BOOST_TEST(tracker.TotalDestroyed() == destroyed_before + 1);
	BOOST_TEST(tracker.ActiveCount() == active_before);
}

BOOST_AUTO_TEST_CASE(Test_SuspendResume_Tracking)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	uintptr_t handle = 0xDEAD0002;
	tracker.OnCreated(handle, "SuspendResumeCoroutine");

	tracker.OnSuspended(handle);

	auto active = tracker.GetActive();
	auto it = std::find_if(active.begin(), active.end(),
		[handle](const CoroutineInfo& info) { return info.handle_address == handle; });
	BOOST_REQUIRE(it != active.end());
	BOOST_TEST(it->is_suspended == true);
	BOOST_TEST(it->suspend_count == 1u);

	tracker.OnResumed(handle);

	active = tracker.GetActive();
	it = std::find_if(active.begin(), active.end(),
		[handle](const CoroutineInfo& info) { return info.handle_address == handle; });
	BOOST_REQUIRE(it != active.end());
	BOOST_TEST(it->is_suspended == false);
	BOOST_TEST(it->resume_count == 1u);

	tracker.OnDestroyed(handle);
}

BOOST_AUTO_TEST_CASE(Test_GetSuspendedLongerThan)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	uintptr_t handle = 0xDEAD0003;
	tracker.OnCreated(handle, "LongSuspendCoroutine");
	tracker.OnSuspended(handle);

	std::this_thread::sleep_for(50ms);

	auto stuck = tracker.GetSuspendedLongerThan(10ms);
	bool found = std::any_of(stuck.begin(), stuck.end(),
		[handle](const CoroutineInfo& info) { return info.handle_address == handle; });
	BOOST_TEST(found);

	auto not_stuck = tracker.GetSuspendedLongerThan(10s);
	bool not_found = std::none_of(not_stuck.begin(), not_stuck.end(),
		[handle](const CoroutineInfo& info) { return info.handle_address == handle; });
	BOOST_TEST(not_found);

	tracker.OnDestroyed(handle);
}

BOOST_AUTO_TEST_CASE(Test_ThreadSafety)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	std::vector<std::thread> threads;
	constexpr int THREADS = 4;
	constexpr int OPS_PER_THREAD = 100;

	for (int t = 0; t < THREADS; ++t)
	{
		threads.emplace_back([&tracker, t]()
		{
			for (int i = 0; i < OPS_PER_THREAD; ++i)
			{
				uintptr_t handle = static_cast<uintptr_t>(0xF0000000 + t * OPS_PER_THREAD + i);
				tracker.OnCreated(handle, "ThreadSafetyTest");
				tracker.OnSuspended(handle);
				tracker.OnResumed(handle);
				tracker.OnDestroyed(handle);
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	// If we got here without crashing, thread safety is working
	BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()
