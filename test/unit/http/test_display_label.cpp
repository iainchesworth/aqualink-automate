#include <boost/test/unit_test.hpp>

#include <nlohmann/json.hpp>

#include "http/json/json_equipment.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(ComputeDisplayLabel_TestSuite)

BOOST_AUTO_TEST_CASE(NoOverride_NoToggle_ReturnsCanonicalLabel)
{
	const nlohmann::json overrides = nlohmann::json::object();
	BOOST_CHECK_EQUAL(HTTP::JSON::ComputeDisplayLabel("Aux5", "Aux5", overrides, false), "Aux5");
}

BOOST_AUTO_TEST_CASE(Override_NoToggle_ReturnsFriendlyName)
{
	const nlohmann::json overrides = { { "Aux5", "Pool Light" } };
	BOOST_CHECK_EQUAL(HTTP::JSON::ComputeDisplayLabel("Aux5", "Aux5", overrides, false), "Pool Light");
}

BOOST_AUTO_TEST_CASE(Toggle_AppendsHardwareId)
{
	const nlohmann::json overrides = { { "Aux5", "Pool Light" } };
	BOOST_CHECK_EQUAL(HTTP::JSON::ComputeDisplayLabel("Aux5", "Aux5", overrides, true), "Pool Light (Aux5)");

	const nlohmann::json none = nlohmann::json::object();
	BOOST_CHECK_EQUAL(HTTP::JSON::ComputeDisplayLabel("Aux5", "Aux5", none, true), "Aux5 (Aux5)");
}

BOOST_AUTO_TEST_CASE(Toggle_NoHardwareId_NoSuffix)
{
	// Devices without a hardware id (pumps/heaters) get no suffix even when the toggle is on.
	const nlohmann::json overrides = nlohmann::json::object();
	BOOST_CHECK_EQUAL(HTTP::JSON::ComputeDisplayLabel("Filter Pump", "", overrides, true), "Filter Pump");
}

BOOST_AUTO_TEST_SUITE_END()
