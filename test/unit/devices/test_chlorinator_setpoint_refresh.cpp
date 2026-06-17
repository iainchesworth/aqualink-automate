#include <chrono>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/chlorinator_setpoint_refresh.h"

using AqualinkAutomate::Devices::ChlorinatorSetpointRefresh;
using Action = AqualinkAutomate::Devices::ChlorinatorSetpointRefresh::Action;

namespace
{
	using namespace std::chrono_literals;

	// A non-trivial base time so "interval since epoch" never accidentally reads as elapsed.
	const ChlorinatorSetpointRefresh::TimePoint BASE = ChlorinatorSetpointRefresh::TimePoint{} + 1000h;

	// Build a configured + startup-complete + timer-seeded refresh, ready for the steady-state
	// matrix tests (mirrors what the device does on entering NormalOperation).
	ChlorinatorSetpointRefresh MakeReady(std::chrono::seconds interval, ChlorinatorSetpointRefresh::TimePoint seeded_at)
	{
		ChlorinatorSetpointRefresh r;
		r.Configure(interval);
		r.MarkStartupComplete();
		r.NotifyScrapeFinished(seeded_at);   // seed the interval timer
		return r;
	}
}

BOOST_AUTO_TEST_SUITE(ChlorinatorSetpointRefresh_TestSuite)

// --- Configuration gating ----------------------------------------------------

BOOST_AUTO_TEST_CASE(Unconfigured_NeverScrapes)
{
	ChlorinatorSetpointRefresh r;          // default-constructed: disabled
	r.MarkStartupComplete();
	BOOST_CHECK(!r.IsEnabled());
	BOOST_CHECK(r.Evaluate(/*emul*/true, /*goal*/false, /*online*/false, BASE) == Action::None);
}

BOOST_AUTO_TEST_CASE(ZeroInterval_Disables)
{
	ChlorinatorSetpointRefresh r;
	r.Configure(0s);
	r.MarkStartupComplete();
	BOOST_CHECK(!r.IsEnabled());
	BOOST_CHECK(r.Evaluate(true, false, false, BASE + 100h) == Action::None);
}

// --- Hard gates --------------------------------------------------------------

BOOST_AUTO_TEST_CASE(NotEmulating_NeverScrapes)
{
	auto r = MakeReady(300s, BASE);
	BOOST_CHECK(r.Evaluate(/*emul*/false, false, false, BASE + 400s) == Action::None);
}

BOOST_AUTO_TEST_CASE(GoalInProgress_Defers)
{
	auto r = MakeReady(300s, BASE);
	BOOST_CHECK(r.Evaluate(true, /*goal*/true, false, BASE + 400s) == Action::None);
}

BOOST_AUTO_TEST_CASE(StartupNotComplete_NeverScrapes)
{
	ChlorinatorSetpointRefresh r;
	r.Configure(300s);
	r.NotifyScrapeFinished(BASE);   // seeded, but startup never marked complete
	BOOST_CHECK(r.Evaluate(true, false, false, BASE + 400s) == Action::None);
}

// --- Timing ------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(IntervalNotElapsed_NoScrape)
{
	auto r = MakeReady(300s, BASE);
	BOOST_CHECK(r.Evaluate(true, false, false, BASE + 100s) == Action::None);
}

BOOST_AUTO_TEST_CASE(IntervalElapsed_Scrapes)
{
	auto r = MakeReady(300s, BASE);
	BOOST_CHECK(r.Evaluate(true, false, false, BASE + 301s) == Action::Scrape);
}

BOOST_AUTO_TEST_CASE(ScrapeStarted_ResetsIntervalTimer)
{
	auto r = MakeReady(300s, BASE);
	BOOST_REQUIRE(r.Evaluate(true, false, false, BASE + 301s) == Action::Scrape);

	r.NotifyScrapeStarted(BASE + 301s);
	BOOST_CHECK(r.Evaluate(true, false, false, BASE + 400s) == Action::None);   // only 99s since start
	BOOST_CHECK(r.Evaluate(true, false, false, BASE + 602s) == Action::Scrape); // 301s since start
}

// --- Comms-recovery one-shot -------------------------------------------------

BOOST_AUTO_TEST_CASE(FirstOnlineSample_IsBaseline_NotRecovery)
{
	auto r = MakeReady(300s, BASE);
	// First observation is online but well within the interval: it only establishes the
	// baseline and must NOT be mistaken for an offline->online recovery.
	BOOST_CHECK(r.Evaluate(true, false, /*online*/true, BASE + 10s) == Action::None);
}

BOOST_AUTO_TEST_CASE(OfflineToOnlineEdge_ForcesScrapeBeforeInterval)
{
	auto r = MakeReady(300s, BASE);
	BOOST_REQUIRE(r.Evaluate(true, false, /*online*/false, BASE + 10s) == Action::None);  // baseline: offline
	BOOST_CHECK(r.Evaluate(true, false, /*online*/true, BASE + 20s) == Action::Scrape);   // recovery edge

	// Starting the scrape consumes the one-shot; no further scrape until the interval elapses.
	r.NotifyScrapeStarted(BASE + 20s);
	BOOST_CHECK(r.Evaluate(true, false, /*online*/true, BASE + 30s) == Action::None);
}

BOOST_AUTO_TEST_CASE(RecoveryEdge_StillDefersWhileBusy)
{
	auto r = MakeReady(300s, BASE);
	BOOST_REQUIRE(r.Evaluate(true, false, false, BASE + 10s) == Action::None);  // baseline offline
	// A recovery edge arrives but a user/SET goal is in flight: must defer (None), and the
	// pending recovery is retained for the next free tick.
	BOOST_CHECK(r.Evaluate(true, /*goal*/true, /*online*/true, BASE + 20s) == Action::None);
	BOOST_CHECK(r.RecoveryPending());
	BOOST_CHECK(r.Evaluate(true, /*goal*/false, /*online*/true, BASE + 30s) == Action::Scrape);
}

// --- Permanent disable -------------------------------------------------------

BOOST_AUTO_TEST_CASE(Disable_PermanentlyStops)
{
	auto r = MakeReady(300s, BASE);
	r.Disable();
	BOOST_CHECK(!r.IsEnabled());
	BOOST_CHECK(r.Evaluate(true, false, false, BASE + 400s) == Action::None);
	// Even a recovery edge must not revive it.
	BOOST_CHECK(r.Evaluate(true, false, true, BASE + 500s) == Action::None);
}

BOOST_AUTO_TEST_SUITE_END()
