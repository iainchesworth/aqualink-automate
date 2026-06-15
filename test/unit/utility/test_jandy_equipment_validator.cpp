#include <algorithm>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "auxillaries/jandy_auxillary_id.h"
#include "auxillaries/jandy_powercenter_mapping.h"
#include "kernel/powercenter.h"
#include "utility/jandy_equipment_validator.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Auxillaries;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Utility;

namespace
{
	// Build a contiguous run of numbered aux ids [first, first+count) as a vector.
	std::vector<JandyAuxillaryIds> AuxRun(uint8_t first, uint8_t count)
	{
		std::vector<JandyAuxillaryIds> ids;
		for (uint8_t i = 0; i < count; ++i)
		{
			ids.push_back(static_cast<JandyAuxillaryIds>(first + i));
		}
		return ids;
	}

	// Full aux id set for an RS model split across power centers (A=7, B/C/D=8 each).
	std::vector<JandyAuxillaryIds> AuxSet(uint8_t total)
	{
		std::vector<JandyAuxillaryIds> ids;
		const uint8_t caps[4] = { 7, 8, 8, 8 };
		const uint8_t base[4] = { 0x01, 0x08, 0x10, 0x18 };
		uint8_t remaining = total;
		for (int pc = 0; pc < 4 && remaining > 0; ++pc)
		{
			const uint8_t n = std::min<uint8_t>(remaining, caps[pc]);
			auto run = AuxRun(base[pc], n);
			ids.insert(ids.end(), run.begin(), run.end());
			remaining -= n;
		}
		return ids;
	}
}

BOOST_AUTO_TEST_SUITE(JandyEquipmentValidator_TestSuite)

// =============================================================================
// PowerCenterForAuxId mapping
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPowerCenterForAuxId_LocalCentreA)
{
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_1) == PowerCenterIds::A);
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_7) == PowerCenterIds::A);
}

BOOST_AUTO_TEST_CASE(TestPowerCenterForAuxId_RemoteCentresBCD)
{
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_B1) == PowerCenterIds::B);
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_B8) == PowerCenterIds::B);
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_C1) == PowerCenterIds::C);
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_C8) == PowerCenterIds::C);
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_D1) == PowerCenterIds::D);
	BOOST_CHECK(PowerCenterForAuxId(JandyAuxillaryIds::Aux_D8) == PowerCenterIds::D);
}

BOOST_AUTO_TEST_CASE(TestPowerCenterForAuxId_ExtraAuxHasNoCentre)
{
	BOOST_CHECK(!PowerCenterForAuxId(JandyAuxillaryIds::ExtraAux).has_value());
}

// =============================================================================
// PowerCenters attribution container
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPowerCenters_AttributionAndCounts)
{
	PowerCenters pcs;
	pcs.Assign(PowerCenterIds::A, "Aux1");
	pcs.Assign(PowerCenterIds::A, "Aux2");
	pcs.Assign(PowerCenterIds::B, "Aux B1");

	BOOST_CHECK(pcs.IsInstalled(PowerCenterIds::A));
	BOOST_CHECK(pcs.IsInstalled(PowerCenterIds::B));
	BOOST_CHECK(!pcs.IsInstalled(PowerCenterIds::C));
	BOOST_CHECK(!pcs.IsInstalled(PowerCenterIds::D));
	BOOST_CHECK_EQUAL(pcs.InstalledCount(), 2);
	BOOST_CHECK_EQUAL(pcs.TotalAuxillaries(), 3);
	BOOST_CHECK_EQUAL(pcs.AuxillariesIn(PowerCenterIds::A).size(), 2);
}

// =============================================================================
// ValidateDiscoveredEquipment - clean matches across power-centre counts
// =============================================================================

BOOST_AUTO_TEST_CASE(TestValidate_SingleCentre_RS8_Clean)
{
	// RS-8: 7 aux, 1 power center.
	auto result = ValidateDiscoveredEquipment(7, 1, AuxSet(7), 0);
	BOOST_CHECK(result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredAuxillaries, 7);
	BOOST_CHECK_EQUAL(result.DiscoveredPowerCenters, 1);
}

BOOST_AUTO_TEST_CASE(TestValidate_TwoCentres_RS16_Clean)
{
	// RS-16 Combo: 15 aux across 2 power centers (A=7 + B=8).
	auto result = ValidateDiscoveredEquipment(15, 2, AuxSet(15), 0);
	BOOST_CHECK(result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredAuxillaries, 15);
	BOOST_CHECK_EQUAL(result.DiscoveredPowerCenters, 2);
}

BOOST_AUTO_TEST_CASE(TestValidate_ThreeCentres_RS24_Clean)
{
	// RS-24 Combo: 23 aux across 3 power centers (A=7 + B=8 + C=8). Not reachable on the
	// simulator (no ControllerType) - validated here against the decoder ground truth.
	auto result = ValidateDiscoveredEquipment(23, 3, AuxSet(23), 0);
	BOOST_CHECK(result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredAuxillaries, 23);
	BOOST_CHECK_EQUAL(result.DiscoveredPowerCenters, 3);
}

BOOST_AUTO_TEST_CASE(TestValidate_FourCentres_RS32_Clean)
{
	// RS-32 Combo: 31 aux across 4 power centers (A=7 + B=8 + C=8 + D=8).
	auto result = ValidateDiscoveredEquipment(31, 4, AuxSet(31), 0);
	BOOST_CHECK(result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredAuxillaries, 31);
	BOOST_CHECK_EQUAL(result.DiscoveredPowerCenters, 4);
}

// =============================================================================
// ValidateDiscoveredEquipment - anomalies
// =============================================================================

BOOST_AUTO_TEST_CASE(TestValidate_Shortfall_IncompleteScrape_Flagged)
{
	// Model expects 7 aux but only 5 discovered (a short/incomplete scrape).
	auto result = ValidateDiscoveredEquipment(7, 1, AuxSet(5), 0);
	BOOST_CHECK(!result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredAuxillaries, 5);
	BOOST_CHECK(!result.Anomalies.empty());
}

BOOST_AUTO_TEST_CASE(TestValidate_DipReconfiguredRelay_StillValidates)
{
	// DIP repurposed one aux relay as a cleaner: 6 numbered aux + 1 reconfigured = 7 total,
	// which still matches the model's 7 aux relays, so validation passes.
	auto result = ValidateDiscoveredEquipment(7, 1, AuxSet(6), 1);
	BOOST_CHECK(result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredAuxillaries, 7);
}

BOOST_AUTO_TEST_CASE(TestValidate_AuxBeyondModelCapacity_Flagged)
{
	// RS-8 declares one power center, but an "Aux B1" (centre B) was attributed - a mis-scrape.
	std::vector<JandyAuxillaryIds> ids = AuxRun(0x01, 7);   // Aux1..Aux7 (centre A)
	ids.push_back(JandyAuxillaryIds::Aux_B1);               // stray centre-B aux
	auto result = ValidateDiscoveredEquipment(7, 1, ids, 0);
	BOOST_CHECK(!result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredPowerCenters, 2);
}

BOOST_AUTO_TEST_CASE(TestValidate_ExtraAuxExcludedFromCount)
{
	// RS-2/10 Dual: 10 numbered aux + a separate "Extra Aux". The Extra Aux is a dedicated
	// shared relay and must NOT count toward the model's 10 aux relays.
	std::vector<JandyAuxillaryIds> ids = AuxSet(10);        // 10 numbered (A=7 + B=3)
	ids.push_back(JandyAuxillaryIds::ExtraAux);
	auto result = ValidateDiscoveredEquipment(10, 2, ids, 0);
	BOOST_CHECK(result.Passed());
	BOOST_CHECK_EQUAL(result.DiscoveredAuxillaries, 10);
}

BOOST_AUTO_TEST_CASE(TestValidate_NoModelDecoded_NoAnomalies)
{
	// Expected aux == 0 (version page not scraped): validation is a no-op, never flags.
	auto result = ValidateDiscoveredEquipment(0, 0, AuxSet(5), 0);
	BOOST_CHECK(result.Passed());
}

BOOST_AUTO_TEST_SUITE_END()
