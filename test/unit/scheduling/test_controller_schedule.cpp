#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <nlohmann/json.hpp>

#include "scheduling/controller_schedule.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Scheduling;

BOOST_AUTO_TEST_SUITE(TestSuite_ControllerSchedule)

BOOST_AUTO_TEST_CASE(ControllerSchedule_Span_Serialises)
{
	ControllerSchedule c;
	c.id = "program-1";
	c.name = "Filter pump";
	c.target = "Filter Pump";
	c.enabled = true;
	c.days_of_week = 0x7f;   // every day
	c.on_hour = 8;
	c.on_minute = 0;
	c.off_hour = 12;
	c.off_minute = 30;

	auto json = ToJson(c);
	BOOST_CHECK_EQUAL(json["id"], "program-1");
	BOOST_CHECK_EQUAL(json["target"], "Filter Pump");
	BOOST_CHECK_EQUAL(json["enabled"], true);
	BOOST_CHECK_EQUAL(json["days_of_week"], 0x7f);
	BOOST_CHECK_EQUAL(json["on_local"], "08:00");
	BOOST_CHECK_EQUAL(json["off_local"], "12:30");
}

BOOST_AUTO_TEST_CASE(ControllerScheduleStatus_StringMapping)
{
	BOOST_CHECK_EQUAL(ControllerScheduleStatusToString(ControllerScheduleStatus::Available), "available");
	BOOST_CHECK_EQUAL(ControllerScheduleStatusToString(ControllerScheduleStatus::PendingCapture), "pending_capture");
	BOOST_CHECK_EQUAL(ControllerScheduleStatusToString(ControllerScheduleStatus::Unsupported), "unsupported");
}

BOOST_AUTO_TEST_CASE(ControllerScheduleStore_DefaultsToPendingAndEmpty)
{
	ControllerScheduleStore store;
	BOOST_CHECK(store.Status() == ControllerScheduleStatus::PendingCapture);
	BOOST_CHECK(store.List().empty());
}

BOOST_AUTO_TEST_CASE(ControllerScheduleStore_ReplaceSwapsSnapshot)
{
	ControllerScheduleStore store;

	std::vector<ControllerSchedule> programs;
	ControllerSchedule c;
	c.id = "program-1";
	c.target = "Filter Pump";
	programs.push_back(c);

	store.Replace(ControllerScheduleStatus::Available, std::move(programs));

	BOOST_CHECK(store.Status() == ControllerScheduleStatus::Available);
	BOOST_REQUIRE_EQUAL(store.List().size(), 1u);
	BOOST_CHECK_EQUAL(store.List().front().target, "Filter Pump");
}

BOOST_AUTO_TEST_SUITE_END()
