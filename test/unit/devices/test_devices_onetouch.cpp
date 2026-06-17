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

// =============================================================================
// SetpointController capability surface (Set Temperature menu actuation)
//
// SetPoolSetpoint/SetSpaSetpoint enqueue a single menu-edit goal serviced in
// NormalOperation. The synchronous contract just confirms the request was
// accepted/queued; the actual value-stepping is driven later by the Navigator.
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSetpoint_Emulated_AcceptsPoolAndSpa)
{
	OneTouchDevice device(device_type, *this, true);

	BOOST_CHECK_EQUAL(static_cast<int>(device.SetPoolSetpoint(82)),
		static_cast<int>(Devices::Capabilities::ActuationResult::Accepted));
}

BOOST_AUTO_TEST_CASE(TestSetpoint_NonEmulated_NotSupported)
{
	// A passive (non-emulated) OneTouch cannot transmit, so it cannot edit a setpoint.
	OneTouchDevice device(device_type, *this, false);

	BOOST_CHECK_EQUAL(static_cast<int>(device.SetSpaSetpoint(100)),
		static_cast<int>(Devices::Capabilities::ActuationResult::NotSupported));
}

// =============================================================================
// Operating-state gate (regression: false "Applied" while the controller is faulted)
//
// ScrapingFaulted / FaultHasOccurred are unrecoverable dead-end states whose queued
// goals are never serviced (the per-frame service step runs only in NormalOperation).
// Every actuation path must refuse honestly (NotSupported) in a fault rather than
// queue a goal and falsely report Accepted. A fresh (pre-fault) emulated device still
// accepts, proving it is the fault -- not the request -- that blocks it.
// =============================================================================

namespace
{
	// Exposes the protected test seam so a test can drive the unrecoverable fault state.
	struct FaultableOneTouchDevice : public OneTouchDevice
	{
		using OneTouchDevice::OneTouchDevice;
		using OneTouchDevice::ForceScrapingFaultedForTest;
	};
}

BOOST_AUTO_TEST_CASE(TestActuate_FaultState_Refused)
{
	using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

	auto aux = std::make_shared<Kernel::AuxillaryDevice>();
	aux->AuxillaryTraits.Set(LabelTrait{}, std::string{ "Pool Light" });
	aux->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Auxillary);
	aux->AuxillaryTraits.Set(Auxillaries::JandyAuxillaryId{}, Auxillaries::JandyAuxillaryIds::Aux_5);

	// Fresh emulated device (ColdStart, not faulted): a toggle is accepted/queued -- proving it
	// is the fault, not the request, that blocks the faulted case below.
	FaultableOneTouchDevice fresh(device_type, *this, true);
	BOOST_CHECK(fresh.ActuateDevice(aux, Capabilities::ActuationAction::Toggle) == Capabilities::ActuationResult::Accepted);

	// Same kind of device, driven into the unrecoverable ScrapingFaulted state: every actuation
	// path refuses honestly. The fault gate runs before any goal is queued, so the calls do not
	// interfere with one another (none of them sets a pending goal).
	FaultableOneTouchDevice faulted(device_type, *this, true);
	faulted.ForceScrapingFaultedForTest();
	BOOST_CHECK(faulted.ActuateDevice(aux, Capabilities::ActuationAction::Toggle) == Capabilities::ActuationResult::NotSupported);
	BOOST_CHECK(faulted.SetPoolSetpoint(82) == Capabilities::ActuationResult::NotSupported);
	BOOST_CHECK(faulted.SetSpaSetpoint(100) == Capabilities::ActuationResult::NotSupported);
	BOOST_CHECK(faulted.SetChlorinatorPercentage(50) == Capabilities::ActuationResult::NotSupported);
	BOOST_CHECK(faulted.SetChlorinatorBoost(true) == Capabilities::ActuationResult::NotSupported);
}

BOOST_AUTO_TEST_CASE(TestSetpoint_RejectsSecondGoalWhileBusy)
{
	// One goal at a time: a second request while the first is still pending is rejected
	// so two cursor walks never interleave on the single shared Navigator.
	OneTouchDevice device(device_type, *this, true);

	BOOST_CHECK_EQUAL(static_cast<int>(device.SetPoolSetpoint(82)),
		static_cast<int>(Devices::Capabilities::ActuationResult::Accepted));
	BOOST_CHECK_EQUAL(static_cast<int>(device.SetSpaSetpoint(100)),
		static_cast<int>(Devices::Capabilities::ActuationResult::NotSupported));
}

BOOST_AUTO_TEST_CASE(TestSetpoint_ControllerPriority_IsLow)
{
	// OneTouch is a Low-priority controller so a direct-command Serial Adapter (High) and
	// the direct-command AqualinkTouch (Medium) are preferred when present (shared by the
	// DeviceActuator / SetpointController / ChlorinatorController mixins).
	OneTouchDevice device(device_type, *this, true);
	BOOST_CHECK_EQUAL(static_cast<int>(device.ControllerPriority()),
		static_cast<int>(Devices::Capabilities::ActuationPriority::Low));
}

// =============================================================================
// ChlorinatorController capability surface (Set AquaPure % via the menu value
// editor; boost via the Boost Pool page). Verified vs onetouch_chlorinator.cap.
// =============================================================================

BOOST_AUTO_TEST_CASE(TestChlorinator_Emulated_AcceptsPercentageAndBoost)
{
	OneTouchDevice device(device_type, *this, true);

	BOOST_CHECK_EQUAL(static_cast<int>(device.SetChlorinatorPercentage(45)),
		static_cast<int>(Devices::Capabilities::ActuationResult::Accepted));
}

BOOST_AUTO_TEST_CASE(TestChlorinatorBoost_Emulated_Accepts)
{
	OneTouchDevice device(device_type, *this, true);

	BOOST_CHECK_EQUAL(static_cast<int>(device.SetChlorinatorBoost(true)),
		static_cast<int>(Devices::Capabilities::ActuationResult::Accepted));
}

BOOST_AUTO_TEST_CASE(TestChlorinator_NonEmulated_NotSupported)
{
	// A passive (non-emulated) OneTouch cannot transmit, so it cannot edit the chlorinator.
	OneTouchDevice device(device_type, *this, false);

	BOOST_CHECK_EQUAL(static_cast<int>(device.SetChlorinatorPercentage(50)),
		static_cast<int>(Devices::Capabilities::ActuationResult::NotSupported));
	BOOST_CHECK_EQUAL(static_cast<int>(device.SetChlorinatorBoost(false)),
		static_cast<int>(Devices::Capabilities::ActuationResult::NotSupported));
}

BOOST_AUTO_TEST_CASE(TestChlorinator_RejectsWhileAnotherGoalBusy)
{
	// One shared Navigator -> a chlorinator edit is rejected while a setpoint goal is pending.
	OneTouchDevice device(device_type, *this, true);

	BOOST_CHECK_EQUAL(static_cast<int>(device.SetPoolSetpoint(82)),
		static_cast<int>(Devices::Capabilities::ActuationResult::Accepted));
	BOOST_CHECK_EQUAL(static_cast<int>(device.SetChlorinatorPercentage(45)),
		static_cast<int>(Devices::Capabilities::ActuationResult::NotSupported));
}

BOOST_AUTO_TEST_SUITE_END()
