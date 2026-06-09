#include <cstdint>
#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/auxillaries/jandy_auxillary_id.h"
#include "jandy/auxillaries/jandy_auxillary_traits_types.h"

#include "kernel/data_hub.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;

namespace
{
	struct OneTouchDeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		OneTouchDeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x41)))
			, data_hub(Find<Kernel::DataHub>())
		{
		}

		// Add an aux device to the DataHub mimicking what a real iAqualink2 seeds:
		// a Jandy aux id plus a (possibly empty) label.
		void SeedAux(Auxillaries::JandyAuxillaryIds aux_id, const std::string& label)
		{
			auto aux = std::make_shared<Kernel::AuxillaryDevice>();
			aux->AuxillaryTraits.Set(Auxillaries::JandyAuxillaryId{}, aux_id);
			if (!label.empty())
			{
				aux->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, label);
			}
			data_hub->Devices.Add(aux);
		}

		std::shared_ptr<JandyDeviceType> device_type;
		std::shared_ptr<Kernel::DataHub> data_hub;
	};
}

BOOST_FIXTURE_TEST_SUITE(OneTouchDevice_TestSuite, OneTouchDeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_Emulated)
{
	BOOST_CHECK_NO_THROW(OneTouchDevice device(device_type, *this, true));
}

BOOST_AUTO_TEST_CASE(TestConstruction_NonEmulated)
{
	BOOST_CHECK_NO_THROW(OneTouchDevice device(device_type, *this, false));
}

// =============================================================================
// Destruction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_CleanAfterConstruction)
{
	{
		OneTouchDevice device(device_type, *this, true);
	}
	BOOST_CHECK(true);
}

// =============================================================================
// Multiple device IDs in the OneTouch range
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DifferentDeviceId)
{
	auto device_type_40 = std::make_shared<JandyDeviceType>(JandyDeviceId(0x40));
	BOOST_CHECK_NO_THROW(OneTouchDevice device(device_type_40, *this, true));
}

// =============================================================================
// IAQ-seeded aux label detection (drives the Label Aux scrape skip)
// =============================================================================

// NEGATIVE: with no aux labels on the DataHub (no IAQ on the bus), the emulated
// OneTouch must NOT skip the Label Aux scrape - the full crawl still runs.
BOOST_AUTO_TEST_CASE(TestSeededLabels_NoLabels_PlansFullScrape)
{
	OneTouchDevice device(device_type, *this, true);

	// Nothing seeded yet.
	BOOST_CHECK(!device.DataHubHasSeededAuxLabels());

	// An aux that exists but carries no label is not evidence of seeded labels.
	SeedAux(Auxillaries::JandyAuxillaryIds::Aux_1, "");
	BOOST_CHECK(!device.DataHubHasSeededAuxLabels());
}

// POSITIVE: when a real iAqualink2 has seeded an aux label onto the DataHub, the
// emulated OneTouch detects it and will skip the Label Aux scrape.
BOOST_AUTO_TEST_CASE(TestSeededLabels_LabelPresent_SkipsScrape)
{
	OneTouchDevice device(device_type, *this, true);

	SeedAux(Auxillaries::JandyAuxillaryIds::Aux_1, "Pool Light");
	BOOST_CHECK(device.DataHubHasSeededAuxLabels());
}

// A whitespace-only label is treated as no label (not a real seeded name).
BOOST_AUTO_TEST_CASE(TestSeededLabels_WhitespaceLabel_PlansFullScrape)
{
	OneTouchDevice device(device_type, *this, true);

	SeedAux(Auxillaries::JandyAuxillaryIds::Aux_2, "   ");
	BOOST_CHECK(!device.DataHubHasSeededAuxLabels());
}

// One labelled aux among several unlabelled ones is enough to trigger the skip.
BOOST_AUTO_TEST_CASE(TestSeededLabels_OneOfManyLabelled_SkipsScrape)
{
	OneTouchDevice device(device_type, *this, true);

	SeedAux(Auxillaries::JandyAuxillaryIds::Aux_1, "");
	SeedAux(Auxillaries::JandyAuxillaryIds::Aux_2, "");
	SeedAux(Auxillaries::JandyAuxillaryIds::Aux_3, "Waterfall");
	BOOST_CHECK(device.DataHubHasSeededAuxLabels());
}

BOOST_AUTO_TEST_SUITE_END()
