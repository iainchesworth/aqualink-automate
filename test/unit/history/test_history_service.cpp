#include <cstdint>
#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/asio/io_context.hpp>

#include "history/history_service.h"
#include "history/sqlite_db.h"
#include "kernel/preferences_hub.h"
#include "options/options_history_options.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;

namespace
{
	Options::History::HistorySettings MemorySettings()
	{
		Options::History::HistorySettings s;
		s.db_path = ":memory:";
		s.retention_days = 90;
		s.flush_seconds = 10;
		return s;
	}
}

//=============================================================================
// RAII SQLite wrapper
//=============================================================================
BOOST_AUTO_TEST_SUITE(TestSuite_SqliteDb)

BOOST_AUTO_TEST_CASE(SqliteDb_PreparedStatementRoundTrip)
{
	History::SqliteDb db(":memory:");
	db.Exec("CREATE TABLE t(id INTEGER PRIMARY KEY, k TEXT, v REAL);");

	{
		History::SqliteStmt insert(db, "INSERT INTO t(k, v) VALUES(?, ?)");
		insert.Bind(1, std::string{ "alpha" });
		insert.Bind(2, 3.5);
		BOOST_CHECK(!insert.Step()); // INSERT yields no row
	}

	History::SqliteStmt select(db, "SELECT k, v FROM t WHERE k = ?");
	select.Bind(1, std::string{ "alpha" });
	BOOST_REQUIRE(select.Step());
	BOOST_CHECK_EQUAL(select.ColumnText(0), "alpha");
	BOOST_CHECK_EQUAL(select.ColumnDouble(1), 3.5);
	BOOST_CHECK(!select.Step());
}

BOOST_AUTO_TEST_CASE(SqliteDb_TransactionRollbackOnScopeExit)
{
	History::SqliteDb db(":memory:");
	db.Exec("CREATE TABLE t(v INTEGER);");

	{
		History::SqliteTransaction txn(db);
		db.Exec("INSERT INTO t(v) VALUES(1);");
		// No Commit() -> destructor rolls back.
	}

	History::SqliteStmt count(db, "SELECT COUNT(*) FROM t");
	BOOST_REQUIRE(count.Step());
	BOOST_CHECK_EQUAL(count.ColumnInt64(0), 0);
}

BOOST_AUTO_TEST_CASE(SqliteDb_OpenInvalidPathThrows)
{
	// A path under a non-existent directory cannot be created.
	BOOST_CHECK_THROW(History::SqliteDb db("R:/this/dir/does/not/exist/h.db"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// HistoryService sampling / downsampling / retention
//=============================================================================
BOOST_FIXTURE_TEST_SUITE(TestSuite_HistoryService, Test::HubLocatorInjector)

BOOST_AUTO_TEST_CASE(RecordNumeric_ThrottlesWithinMinInterval)
{
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());

	std::int64_t now = 1'000'000;
	service.SetClock([&now] { return now; });
	service.Start();

	service.RecordNumeric("temp/pool", "C", 20.0);           // buffered
	now += 30;                                                // within 60s
	service.RecordNumeric("temp/pool", "C", 21.0);           // throttled
	now += 40;                                                // now 70s after first
	service.RecordNumeric("temp/pool", "C", 22.0);           // buffered

	auto series = service.ListSeries();
	BOOST_REQUIRE_EQUAL(series.size(), 1u);
	BOOST_CHECK_EQUAL(series.front().key, "temp/pool");
	BOOST_CHECK_EQUAL(series.front().unit, "C");
	BOOST_CHECK_EQUAL(series.front().count, 2); // throttled middle sample dropped
}

BOOST_AUTO_TEST_CASE(RecordNumeric_HeartbeatBypassesThrottle)
{
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());

	std::int64_t now = 1'000'000;
	service.SetClock([&now] { return now; });
	service.Start();

	service.RecordNumeric("temp/pool", "C", 20.0);                         // buffered
	now += 5;
	service.RecordNumeric("temp/pool", "C", 20.0, /*is_heartbeat=*/true);  // bypass -> buffered

	auto series = service.ListSeries();
	BOOST_REQUIRE_EQUAL(series.size(), 1u);
	BOOST_CHECK_EQUAL(series.front().count, 2);
}

BOOST_AUTO_TEST_CASE(QuerySeries_BucketAveragedDownsample)
{
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());

	std::int64_t now = 0;
	service.SetClock([&now] { return now; });
	service.Start();

	// Four samples at ts 0,10,20,30 -> values 2,4,6,8 (heartbeat bypasses throttle).
	const std::int64_t ts[] = { 0, 10, 20, 30 };
	const double vals[] = { 2.0, 4.0, 6.0, 8.0 };
	for (int i = 0; i < 4; ++i)
	{
		now = ts[i];
		service.RecordNumeric("temp/pool", "C", vals[i], /*is_heartbeat=*/true);
	}

	// from=0,to=40,max_points=2 -> bucket=20. floor(ts/20)*20: {0,10}->0, {20,30}->20.
	auto points = service.QuerySeries("temp/pool", 0, 40, 2);
	BOOST_REQUIRE_EQUAL(points.size(), 2u);
	BOOST_CHECK_EQUAL(points[0].ts, 0);
	BOOST_CHECK_EQUAL(points[0].value, 3.0); // avg(2,4)
	BOOST_CHECK_EQUAL(points[1].ts, 20);
	BOOST_CHECK_EQUAL(points[1].value, 7.0); // avg(6,8)
}

BOOST_AUTO_TEST_CASE(QuerySeries_UnknownKeyEmpty_KnownKeyExists)
{
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());
	std::int64_t now = 500;
	service.SetClock([&now] { return now; });
	service.Start();

	service.RecordNumeric("chem/salt_ppm", "ppm", 3200.0, true);
	service.Flush();

	BOOST_CHECK(service.SeriesExists("chem/salt_ppm"));
	BOOST_CHECK(!service.SeriesExists("nope/missing"));
}

BOOST_AUTO_TEST_CASE(PurgeOld_DropsSamplesBeyondRetention)
{
	boost::asio::io_context io;
	auto settings = MemorySettings();
	settings.retention_days = 90;
	History::HistoryService service(io, *this, settings);

	std::int64_t now = 0;
	service.SetClock([&now] { return now; });
	service.Start();

	// PurgeOld reads retention live from PreferencesHub (seeded from the CLI).
	Find<Kernel::PreferencesHub>()->HistoryRetentionDays = 90;

	// One sample 100 days ago, one "now".
	const std::int64_t reference = 1'000'000'000;
	now = reference - (100 * 86400);
	service.RecordNumeric("temp/pool", "C", 10.0, true);
	now = reference;
	service.RecordNumeric("temp/pool", "C", 20.0, true);
	service.Flush();

	service.PurgeOld(); // cutoff = now - 90d -> the 100-day-old row is removed.

	auto series = service.ListSeries();
	BOOST_REQUIRE_EQUAL(series.size(), 1u);
	BOOST_CHECK_EQUAL(series.front().count, 1);
}

BOOST_AUTO_TEST_CASE(RecordState_StoresTransitionsAsStateSeries)
{
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());
	std::int64_t now = 42;
	service.SetClock([&now] { return now; });
	service.Start();

	service.RecordState("device/pool_pump/state", 1.0);
	now += 5;
	service.RecordState("device/pool_pump/state", 0.0); // transitions are never throttled

	auto series = service.ListSeries();
	BOOST_REQUIRE_EQUAL(series.size(), 1u);
	BOOST_CHECK_EQUAL(series.front().key, "device/pool_pump/state");
	BOOST_CHECK_EQUAL(series.front().unit, "state");
	BOOST_CHECK_EQUAL(series.front().count, 2);
}

BOOST_AUTO_TEST_CASE(RecordDeviceState_KeysByUuidAndCarriesLabel)
{
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());
	std::int64_t now = 100;
	service.SetClock([&now] { return now; });
	service.Start();

	service.RecordDeviceState("device/uuid-aux5", "Pool Light", 1.0);

	auto series = service.ListSeries();
	BOOST_REQUIRE_EQUAL(series.size(), 1u);
	BOOST_CHECK_EQUAL(series.front().key, "device/uuid-aux5");
	BOOST_CHECK_EQUAL(series.front().unit, "state");
	BOOST_CHECK_EQUAL(series.front().label, "Pool Light");
	BOOST_CHECK_EQUAL(series.front().count, 1);
}

BOOST_AUTO_TEST_CASE(RecordDeviceState_RelabelUpdatesInPlaceWithoutDuplicating)
{
	// A device boots as "Aux5" then is renamed "Pool Light" once discovered. The
	// UUID key is stable across the rename, so a single series accumulates and the
	// most-recent label wins — this is the duplicate-in-the-filter fix.
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());
	std::int64_t now = 100;
	service.SetClock([&now] { return now; });
	service.Start();

	service.RecordDeviceState("device/uuid-1", "Aux5", 1.0);
	now += 5;
	service.RecordDeviceState("device/uuid-1", "Pool Light", 0.0);

	auto series = service.ListSeries();
	BOOST_REQUIRE_EQUAL(series.size(), 1u);
	BOOST_CHECK_EQUAL(series.front().key, "device/uuid-1");
	BOOST_CHECK_EQUAL(series.front().label, "Pool Light");
	BOOST_CHECK_EQUAL(series.front().count, 2);
}

BOOST_AUTO_TEST_CASE(RecordDeviceState_FoldsLegacyLabelKeyedSeries)
{
	// Simulate an old database holding a legacy label-keyed series. The first
	// UUID-keyed recording with the matching label folds the legacy samples into
	// the canonical series and drops the legacy row, leaving exactly one series.
	boost::asio::io_context io;
	History::HistoryService service(io, *this, MemorySettings());
	std::int64_t now = 100;
	service.SetClock([&now] { return now; });
	service.Start();

	service.RecordState("device/pool_light/state", 1.0);   // legacy scheme
	now += 5;
	service.RecordState("device/pool_light/state", 0.0);
	service.Flush();

	now += 5;
	service.RecordDeviceState("device/uuid-1", "Pool Light", 1.0);   // canonical scheme
	service.Flush();

	auto series = service.ListSeries();
	BOOST_REQUIRE_EQUAL(series.size(), 1u);
	BOOST_CHECK_EQUAL(series.front().key, "device/uuid-1");
	BOOST_CHECK_EQUAL(series.front().label, "Pool Light");
	BOOST_CHECK_EQUAL(series.front().count, 3);   // 2 legacy + 1 new, merged
}

BOOST_AUTO_TEST_SUITE_END()
