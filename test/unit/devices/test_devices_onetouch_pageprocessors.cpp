#include <boost/test/unit_test.hpp>

#include "support/unit_test_onetouchdevice.h"
#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;

BOOST_FIXTURE_TEST_SUITE(PageProcessor_EquipmentOnOff_TestSuite, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(TestAuxillariesHeatersPumpsAreRegistered)
{
	auto& config = Config();

	BOOST_REQUIRE_EQUAL(config.PoolConfiguration, Config::PoolConfigurations::Unknown);
	BOOST_REQUIRE_EQUAL(config.SystemBoard, Config::SystemBoards::Unknown);

	InitialiseOneTouchDevice();

	BOOST_REQUIRE_NE(config.PoolConfiguration, Config::PoolConfigurations::Unknown);
	BOOST_REQUIRE_NE(config.SystemBoard, Config::SystemBoards::Unknown);

	EquipmentOnOff_Page1();

	BOOST_CHECK_EQUAL(7, config.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, config.Heaters().size());
	BOOST_CHECK_EQUAL(2, config.Pumps().size());

	///FIXME Check the actual devices are as expected.

	EquipmentOnOff_Page2();

	BOOST_CHECK_EQUAL(15, config.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, config.Heaters().size());
	BOOST_CHECK_EQUAL(2, config.Pumps().size());

	///FIXME Check the actual devices are as expected.

	EquipmentOnOff_Page3();

	BOOST_CHECK_EQUAL(15, config.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, config.Heaters().size());
	BOOST_CHECK_EQUAL(2, config.Pumps().size());

	///FIXME Check the actual devices are as expected.
}

BOOST_AUTO_TEST_SUITE_END()
