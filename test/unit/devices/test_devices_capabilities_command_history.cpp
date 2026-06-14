#include <cstddef>
#include <string>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "devices/capabilities/command_history.h"

using namespace AqualinkAutomate::Devices::Capabilities;

namespace
{
	// CommandHistory has a protected destructor (it is only ever destroyed as part of
	// the owning device), so exercise it through a trivial derived test double.
	class TestCommandHistory : public CommandHistory {};
}

BOOST_AUTO_TEST_SUITE(TestSuite_DeviceCapabilities_CommandHistory)

BOOST_AUTO_TEST_CASE(Empty_DescribesAsEmptyArray)
{
	TestCommandHistory h;
	const auto j = h.DescribeRecentCommands();
	BOOST_CHECK(j.is_array());
	BOOST_CHECK_EQUAL(j.size(), 0u);
}

BOOST_AUTO_TEST_CASE(RecordsAreReturnedOldestFirstWithFields)
{
	TestCommandHistory h;
	h.RecordCommand("toggle 'Pool Light'", "Success");
	h.RecordCommand("set the pool setpoint to 30", "InvalidValue");

	const auto j = h.DescribeRecentCommands();
	BOOST_REQUIRE_EQUAL(j.size(), 2u);

	// Oldest first.
	BOOST_CHECK_EQUAL(j[0]["description"].get<std::string>(), "toggle 'Pool Light'");
	BOOST_CHECK_EQUAL(j[0]["outcome"].get<std::string>(), "Success");
	BOOST_CHECK_EQUAL(j[1]["description"].get<std::string>(), "set the pool setpoint to 30");
	BOOST_CHECK_EQUAL(j[1]["outcome"].get<std::string>(), "InvalidValue");

	// Each entry carries a timestamp and a non-negative age.
	BOOST_CHECK(j[0].contains("ts"));
	BOOST_REQUIRE(j[0].contains("age_seconds"));
	BOOST_CHECK_GE(j[0]["age_seconds"].get<long long>(), 0);
}

BOOST_AUTO_TEST_CASE(RingBufferEvictsOldestPastMaxEntries)
{
	TestCommandHistory h;
	const std::size_t total = CommandHistory::MAX_ENTRIES + 4U;
	for (std::size_t i = 0; i < total; ++i)
	{
		h.RecordCommand("cmd " + std::to_string(i), "Success");
	}

	const auto j = h.DescribeRecentCommands();
	BOOST_REQUIRE_EQUAL(j.size(), CommandHistory::MAX_ENTRIES);

	// The oldest (cmd 0..3) were evicted; the retained window starts at cmd 4 and
	// ends at the newest command.
	BOOST_CHECK_EQUAL(j.front()["description"].get<std::string>(), "cmd 4");
	BOOST_CHECK_EQUAL(j.back()["description"].get<std::string>(), "cmd " + std::to_string(total - 1U));
}

BOOST_AUTO_TEST_SUITE_END()
