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
				uintptr_t handle = 0xF0000000ULL + static_cast<uintptr_t>(t * OPS_PER_THREAD + i);
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

BOOST_AUTO_TEST_CASE(Test_SuspendNonexistentHandle_Safe)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	// Suspending a handle that was never created should not crash
	tracker.OnSuspended(0xBADBAD01);
	BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(Test_ResumeNonexistentHandle_Safe)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	// Resuming a handle that was never created should not crash
	tracker.OnResumed(0xBADBAD02);
	BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(Test_MultipleSuspendResume_AccumulateCounts)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	uintptr_t handle = 0xDEAD0010;
	tracker.OnCreated(handle, "MultiCycleCoroutine");

	for (int i = 0; i < 5; ++i)
	{
		tracker.OnSuspended(handle);
		tracker.OnResumed(handle);
	}

	auto active = tracker.GetActive();
	auto it = std::find_if(active.begin(), active.end(),
		[handle](const CoroutineInfo& info) { return info.handle_address == handle; });
	BOOST_REQUIRE(it != active.end());
	BOOST_TEST(it->suspend_count == 5u);
	BOOST_TEST(it->resume_count == 5u);
	BOOST_TEST(it->is_suspended == false);

	tracker.OnDestroyed(handle);
}

BOOST_AUTO_TEST_CASE(Test_GetActive_ReturnsOnlyNonDestroyed)
{
	auto& tracker = CoroutineLifetimeTracker::Instance();

	uintptr_t handle1 = 0xDEAD0020;
	uintptr_t handle2 = 0xDEAD0021;
	uintptr_t handle3 = 0xDEAD0022;

	tracker.OnCreated(handle1, "ActiveTest1");
	tracker.OnCreated(handle2, "ActiveTest2");
	tracker.OnCreated(handle3, "ActiveTest3");

	tracker.OnDestroyed(handle2);

	auto active = tracker.GetActive();

	bool found_h1 = std::any_of(active.begin(), active.end(),
		[handle1](const CoroutineInfo& info) { return info.handle_address == handle1; });
	bool found_h2 = std::any_of(active.begin(), active.end(),
		[handle2](const CoroutineInfo& info) { return info.handle_address == handle2; });
	bool found_h3 = std::any_of(active.begin(), active.end(),
		[handle3](const CoroutineInfo& info) { return info.handle_address == handle3; });

	BOOST_TEST(found_h1);
	BOOST_TEST(!found_h2);
	BOOST_TEST(found_h3);

	tracker.OnDestroyed(handle1);
	tracker.OnDestroyed(handle3);
}

BOOST_AUTO_TEST_SUITE_END()
