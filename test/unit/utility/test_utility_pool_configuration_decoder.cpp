#include <cstdint>

#include <boost/test/unit_test.hpp>

#include "kernel/pool_configurations.h"
#include "kernel/system_boards.h"
#include "utility/jandy_pool_configuration_decoder.h"

using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(PoolConfigurationDecoder_TestSuite)

// =============================================================================
// Single Body configurations
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDecode_RS4_Only)
{
	PoolConfigurationDecoder decoder("RS-4 Only");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::SingleBody);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS4_Only);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 3);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestDecode_RS6_Only)
{
	PoolConfigurationDecoder decoder("RS-6 Only");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::SingleBody);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS6_Only);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 5);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestDecode_RS8_Only)
{
	PoolConfigurationDecoder decoder("RS-8 Only");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::SingleBody);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS8_Only);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 7);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestDecode_PD4_Only)
{
	PoolConfigurationDecoder decoder("PD-4 Only");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::SingleBody);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::PD4_Only);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 3);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestDecode_PD8_Only)
{
	PoolConfigurationDecoder decoder("PD-8 Only");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::SingleBody);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::PD8_Only);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 7);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

// =============================================================================
// Dual Body Shared Equipment configurations
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDecode_RS8_Combo)
{
	PoolConfigurationDecoder decoder("RS-8 Combo");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::DualBody_SharedEquipment);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS8_Combo);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 7);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestDecode_RS12_Combo)
{
	PoolConfigurationDecoder decoder("RS-12 Combo");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::DualBody_SharedEquipment);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS12_Combo);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 11);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 2);
}

BOOST_AUTO_TEST_CASE(TestDecode_RS32_Combo)
{
	PoolConfigurationDecoder decoder("RS-32 Combo");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::DualBody_SharedEquipment);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS32_Combo);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 31);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 4);
}

// =============================================================================
// Dual Body Dual Equipment configurations
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDecode_RS2_6_Dual)
{
	PoolConfigurationDecoder decoder("RS-2/6 Dual");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::DualBody_DualEquipment);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS2_6_Dual);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 6);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestDecode_RS2_30_Dual)
{
	PoolConfigurationDecoder decoder("RS-2/30 Dual");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::DualBody_DualEquipment);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::RS2_30_Dual);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 30);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 4);
}

// =============================================================================
// Unknown configurations
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDecode_UnknownPanelType_FallsBackToUnknown)
{
	PoolConfigurationDecoder decoder("NONEXISTENT-PANEL");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::Unknown);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::Unknown);
	BOOST_CHECK_EQUAL(decoder.AuxillaryCount(), 1);
	BOOST_CHECK_EQUAL(decoder.PowerCenterCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestDecode_EmptyString_FallsBackToUnknown)
{
	PoolConfigurationDecoder decoder("");
	BOOST_CHECK(decoder.Configuration() == PoolConfigurations::Unknown);
	BOOST_CHECK(decoder.SystemBoard() == SystemBoards::Unknown);
}

BOOST_AUTO_TEST_SUITE_END()
