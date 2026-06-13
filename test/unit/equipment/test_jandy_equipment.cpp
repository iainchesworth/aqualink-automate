#include <memory>
#include <span>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "equipment/jandy_equipment.h"
#include "interfaces/idevice.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_ids.h"

#include "support/unit_test_hublocatorinjector.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;

namespace
{
	// Deserialise a framed+checksummed Jandy ACK addressed to `destination`,
	// then fire it through JandyMessage_Ack's static signal so any connected
	// JandyEquipment slot (wired via the variadic ConnectIdentify fold) runs
	// IdentifyAndAddDevice exactly as it would on the wire.
	void EmitAckTo(uint8_t destination)
	{
		// Two payload bytes => the frame is long enough for Ack to deserialise
		// (it reads indices 4 and 5), which is what sets the message Destination.
		const auto frame = Test::MessageBuilder::CreateValidChecksummedMessage(destination, 0x01, { 0x00, 0x00 });

		Messages::JandyMessage_Ack ack;
		const auto byte_span = std::as_bytes(std::span<const uint8_t>(frame.data(), frame.size()));
		BOOST_REQUIRE(ack.Deserialize(byte_span));
		BOOST_REQUIRE_EQUAL(static_cast<unsigned>(ack.Destination().Id()()), static_cast<unsigned>(destination));

		(*Messages::JandyMessage_Ack::GetSignal())(ack);
	}

	std::size_t DeviceCount(const Kernel::EquipmentHub& hub)
	{
		std::size_t count = 0;
		hub.ForEachDevice([&count](const Interfaces::IDevice&) { ++count; });
		return count;
	}
}

BOOST_AUTO_TEST_SUITE(JandyEquipment_TestSuite)

// =============================================================================
// Dispatch: a supported destination class creates exactly one device and a
// duplicate destination is not re-added (exercises the variadic ConnectIdentify
// fold + IdentifyAndAddDevice + the single-lookup stats path).
// =============================================================================

BOOST_AUTO_TEST_CASE(SupportedClass_AddsDeviceOnce_DuplicateNotReadded)
{
	Test::HubLocatorInjector hub_locator;
	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
	auto stats_hub = hub_locator.Find<Kernel::StatisticsHub>();

	Equipment::JandyEquipment equipment(hub_locator);

	BOOST_REQUIRE_EQUAL(DeviceCount(*equipment_hub), 0U);

	// 0x38 is an LX_Heater id -> one HeaterDevice is created.
	EmitAckTo(0x38);
	BOOST_CHECK_EQUAL(DeviceCount(*equipment_hub), 1U);

	// Same destination again -> DeviceExists short-circuits, no second device.
	EmitAckTo(0x38);
	BOOST_CHECK_EQUAL(DeviceCount(*equipment_hub), 1U);

	// Both ACKs were still counted in the per-id statistics.
	BOOST_CHECK_EQUAL(stats_hub->MessageCounts[Messages::JandyMessageIds::Ack].Count(), 2U);
}

// =============================================================================
// Unsupported device classes are tolerated (no device created) and, with the
// once-per-class rate limiting, repeated messages to the same unsupported class
// do not create devices or crash.
// =============================================================================

BOOST_AUTO_TEST_CASE(UnsupportedClass_CreatesNoDevice_AndIsRateLimited)
{
	Test::HubLocatorInjector hub_locator;
	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
	auto stats_hub = hub_locator.Find<Kernel::StatisticsHub>();

	Equipment::JandyEquipment equipment(hub_locator);

	// 0x84 / 0x85 are Chem_Analyzer ids -> recognised at the id layer but with no device
	// case in IdentifyAndAddDevice, so no device is created. (SpaRemote 0x20 is no longer a
	// valid example here -- it is now recognised as a spaside-remote keypad.)
	EmitAckTo(0x84);
	EmitAckTo(0x85);

	BOOST_CHECK_EQUAL(DeviceCount(*equipment_hub), 0U);
	BOOST_CHECK_EQUAL(stats_hub->MessageCounts[Messages::JandyMessageIds::Ack].Count(), 2U);
}

// =============================================================================
// Spaside remotes are recognised: "Dual Spa Switch" (2x4rem, 0x10) and "Spa Link"
// (8button, 0x20) are spa-side keypads and each gets a device on the bus.
// =============================================================================

BOOST_AUTO_TEST_CASE(SpasideRemotes_AreRecognisedAsDevices)
{
	Test::HubLocatorInjector hub_locator;
	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();

	Equipment::JandyEquipment equipment(hub_locator);

	EmitAckTo(0x10);   // Dual Spa Switch (2x4rem)
	EmitAckTo(0x20);   // Spa Link (8button)

	BOOST_CHECK_EQUAL(DeviceCount(*equipment_hub), 2U);
}

// =============================================================================
// Lifetime: JandyEquipment holds a NON-owning raw pointer to the EquipmentHub
// (no shared_ptr back-edge). Once the equipment and the owning HubLocator go
// out of scope, a weak_ptr to the hub must expire -- proving there is no
// reference cycle keeping the hub (and, transitively, the equipment/devices)
// alive at shutdown.
// =============================================================================

BOOST_AUTO_TEST_CASE(NoReferenceCycle_HubExpiresAfterScope)
{
	std::weak_ptr<Kernel::EquipmentHub> hub_observer;

	{
		Test::HubLocatorInjector hub_locator;
		auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
		hub_observer = equipment_hub;

		Equipment::JandyEquipment equipment(hub_locator);

		// Drive a message so the equipment actually touches the hub via its raw
		// pointer (a shared_ptr back-edge, if reintroduced, would be taken here).
		EmitAckTo(0x38);
		BOOST_REQUIRE(!hub_observer.expired());

		// Release the local strong reference; the locator still owns the hub.
		equipment_hub.reset();
		BOOST_REQUIRE(!hub_observer.expired());
	}

	// Equipment + locator destroyed: nothing should still reference the hub.
	BOOST_CHECK(hub_observer.expired());
}

BOOST_AUTO_TEST_SUITE_END()
