#include <chrono>
#include <string>

#include <boost/test/unit_test.hpp>

#include "devices/capabilities/restartable.h"

using namespace AqualinkAutomate::Devices::Capabilities;

namespace
{
	// Test double: exposes the protected watchdog controls and injects a controllable clock
	// so the deadline logic can be exercised without real-time waits.
	class TestRestartable : public Restartable
	{
	public:
		explicit TestRestartable(std::chrono::seconds timeout)
			: Restartable(timeout, /*delayed_start=*/true)
		{
		}

		void SetNow(std::chrono::steady_clock::time_point now) { m_Now = now; }

		using Restartable::Start;
		using Restartable::Kick;
		using Restartable::Stop;
		using Restartable::IsRunning;

		// Drives the deadline check at the injected clock value (PollAll() supplies the
		// shared 'now' in production; the tests pass m_Now to exercise the same path).
		void CheckWatchdogNow() { CheckWatchdog(m_Now); }

		int TimeoutFiredCount() const { return m_FiredCount; }

	protected:
		void WatchdogTimeoutOccurred() override { ++m_FiredCount; }
		std::chrono::steady_clock::time_point Now() const override { return m_Now; }
		std::string WatchdogName() const override { return "test device"; }

	private:
		std::chrono::steady_clock::time_point m_Now{};
		int m_FiredCount{ 0 };
	};

	constexpr std::chrono::seconds TIMEOUT{ 30 };
	const std::chrono::steady_clock::time_point T0{ std::chrono::seconds{ 1000 } };
}

BOOST_AUTO_TEST_SUITE(TestSuite_DeviceCapabilities_Restartable)

BOOST_AUTO_TEST_CASE(Watchdog_FiresAfterTimeoutWithoutKick)
{
	TestRestartable wd(TIMEOUT);
	wd.SetNow(T0);
	wd.Start();
	BOOST_CHECK(wd.IsRunning());

	// Just inside the timeout window -> still alive, no fire.
	wd.SetNow(T0 + std::chrono::seconds{ 29 });
	wd.CheckWatchdogNow();
	BOOST_CHECK(wd.IsRunning());
	BOOST_CHECK_EQUAL(0, wd.TimeoutFiredCount());

	// Past the timeout -> fires once and marks the device not running.
	wd.SetNow(T0 + std::chrono::seconds{ 31 });
	wd.CheckWatchdogNow();
	BOOST_CHECK(!wd.IsRunning());
	BOOST_CHECK_EQUAL(1, wd.TimeoutFiredCount());
}

BOOST_AUTO_TEST_CASE(Watchdog_KickResetsDeadline)
{
	TestRestartable wd(TIMEOUT);
	wd.SetNow(T0);
	wd.Start();

	// A kick at +20s resets the deadline to +20s.
	wd.SetNow(T0 + std::chrono::seconds{ 20 });
	wd.Kick();

	// +45s is only 25s since the kick -> still alive.
	wd.SetNow(T0 + std::chrono::seconds{ 45 });
	wd.CheckWatchdogNow();
	BOOST_CHECK(wd.IsRunning());
	BOOST_CHECK_EQUAL(0, wd.TimeoutFiredCount());

	// +51s is 31s since the kick -> fires.
	wd.SetNow(T0 + std::chrono::seconds{ 51 });
	wd.CheckWatchdogNow();
	BOOST_CHECK(!wd.IsRunning());
	BOOST_CHECK_EQUAL(1, wd.TimeoutFiredCount());
}

BOOST_AUTO_TEST_CASE(Watchdog_StoppedDoesNotFire)
{
	TestRestartable wd(TIMEOUT);
	wd.SetNow(T0);
	wd.Start();
	wd.Stop();
	BOOST_CHECK(!wd.IsRunning());

	wd.SetNow(T0 + std::chrono::seconds{ 1000 });
	wd.CheckWatchdogNow();
	BOOST_CHECK_EQUAL(0, wd.TimeoutFiredCount());
}

BOOST_AUTO_TEST_CASE(Watchdog_PollAllSweepsLiveInstancesWithSharedClock)
{
	// PollAll() reads the real steady_clock once and shares it across every live instance.
	// The test double's injected clock stays at the epoch, so the last Kick() (from Start())
	// is far enough in the past that the shared real 'now' is well beyond a zero timeout,
	// proving the deadline is evaluated against PollAll()'s clock rather than each instance's.
	TestRestartable wd(std::chrono::seconds{ 0 });
	wd.SetNow(std::chrono::steady_clock::time_point{});
	wd.Start();
	BOOST_CHECK(wd.IsRunning());

	Restartable::PollAll();

	BOOST_CHECK(!wd.IsRunning());
	BOOST_CHECK_EQUAL(1, wd.TimeoutFiredCount());

	// A second sweep must not re-fire once the instance has been marked not running.
	Restartable::PollAll();
	BOOST_CHECK_EQUAL(1, wd.TimeoutFiredCount());
}

BOOST_AUTO_TEST_SUITE_END()
