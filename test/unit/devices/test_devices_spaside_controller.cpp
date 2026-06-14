#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/spaside_remote_controller.h"
#include "jandy/devices/spaside_remote_device.h"
#include "kernel/equipment_hub.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using PressResult = Interfaces::ISpasideRemoteController::PressResult;

//=============================================================================
// SpasideRemoteController: enumerates spa-side remotes from the EquipmentHub and
// injects button presses on EMULATED remotes (a passively-observed real remote
// cannot be actuated).
//=============================================================================

namespace
{
	struct ControllerFixture : public Test::HubLocatorInjector
	{
		ControllerFixture() : hub(std::make_shared<Kernel::EquipmentHub>()) {}

		void AddRemote(uint8_t address, bool emulated)
		{
			auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(address));
			hub->AddDevice(std::make_unique<SpasideRemoteDevice>(id, *this, emulated));
		}

		std::shared_ptr<Kernel::EquipmentHub> hub;
	};
}

BOOST_FIXTURE_TEST_SUITE(SpasideRemoteController_TestSuite, ControllerFixture)

BOOST_AUTO_TEST_CASE(Remotes_ListsEachRemoteWithClassAndButtonCount)
{
	AddRemote(0x10, /*emulated*/ false);   // Dual Spa Switch (real, observed)
	AddRemote(0x20, /*emulated*/ true);    // Spa Link (emulated)

	SpasideRemoteController controller(hub);
	const auto remotes = controller.Remotes();

	BOOST_REQUIRE_EQUAL(remotes.size(), 2u);

	// Order is hub-iteration order; index by address rather than assume it.
	auto find = [&](uint8_t addr) -> const Interfaces::ISpasideRemoteController::RemoteState*
	{
		for (const auto& r : remotes) { if (r.address == addr) { return &r; } }
		return nullptr;
	};

	const auto* dual = find(0x10);
	BOOST_REQUIRE(dual != nullptr);
	BOOST_CHECK_EQUAL(dual->device_class, "DualSpaSwitch");
	BOOST_CHECK_EQUAL(dual->emulated, false);
	BOOST_CHECK_EQUAL(static_cast<int>(dual->button_count), 8);

	const auto* link = find(0x20);
	BOOST_REQUIRE(link != nullptr);
	BOOST_CHECK_EQUAL(link->device_class, "SpaRemote");
	BOOST_CHECK_EQUAL(link->emulated, true);
	BOOST_CHECK_EQUAL(static_cast<int>(link->button_count), 9);
}

BOOST_AUTO_TEST_CASE(PressButton_EmulatedRemote_Succeeds)
{
	AddRemote(0x20, /*emulated*/ true);
	SpasideRemoteController controller(hub);

	BOOST_CHECK(controller.PressButton(0x20, 2) == PressResult::Success);
}

BOOST_AUTO_TEST_CASE(PressButton_RealRemote_IsRejectedAsNotEmulated)
{
	// We only observe a real remote on the bus; we are not its transmitter, so we cannot actuate it.
	AddRemote(0x10, /*emulated*/ false);
	SpasideRemoteController controller(hub);

	BOOST_CHECK(controller.PressButton(0x10, 1) == PressResult::NotEmulated);
}

BOOST_AUTO_TEST_CASE(PressButton_UnknownAddress_IsRemoteNotFound)
{
	AddRemote(0x20, /*emulated*/ true);
	SpasideRemoteController controller(hub);

	BOOST_CHECK(controller.PressButton(0x21, 1) == PressResult::RemoteNotFound);
}

BOOST_AUTO_TEST_CASE(PressButton_OutOfRange_IsInvalidButton)
{
	AddRemote(0x20, /*emulated*/ true);   // Spa Link: 9 buttons
	SpasideRemoteController controller(hub);

	BOOST_CHECK(controller.PressButton(0x20, 0) == PressResult::InvalidButton);    // 0 = idle, not a press
	BOOST_CHECK(controller.PressButton(0x20, 10) == PressResult::InvalidButton);   // beyond 9
}

BOOST_AUTO_TEST_CASE(PressButton_BoundaryButtons_AreValid)
{
	AddRemote(0x20, /*emulated*/ true);   // Spa Link: 1..9 valid
	SpasideRemoteController controller(hub);

	BOOST_CHECK(controller.PressButton(0x20, 1) == PressResult::Success);
	BOOST_CHECK(controller.PressButton(0x20, 9) == PressResult::Success);
}

BOOST_AUTO_TEST_CASE(DualSpaSwitch_EmulatesBothInterfaceBoardSwitches)
{
	// The 6588 Dual Spa Side Interface board (DualSpaSwitch at 0x10) bridges TWO physical
	// spa-side switches onto the bus: switch #2 = button codes 1-4, switch #3 = codes 5-8 (the
	// "2x4"). Emulating it must accept the full 1-8 range so a user without the board can add a
	// "fake" switch #2 AND switch #3. (See docs/alwin32/spaside-remotes.md, Sheet #6873/PN 6588.)
	AddRemote(0x10, /*emulated*/ true);
	SpasideRemoteController controller(hub);

	const auto remotes = controller.Remotes();
	BOOST_REQUIRE_EQUAL(remotes.size(), 1u);
	BOOST_CHECK_EQUAL(remotes[0].device_class, "DualSpaSwitch");
	BOOST_CHECK_EQUAL(static_cast<int>(remotes[0].button_count), 8);

	// Switch #2 (codes 1-4) and switch #3 (codes 5-8) all actuate on the emulated board.
	for (int code : { 1, 4, 5, 8 })
	{
		BOOST_CHECK(controller.PressButton(0x10, static_cast<uint8_t>(code)) == PressResult::Success);
	}

	// Beyond the 8 interface-board codes is rejected.
	BOOST_CHECK(controller.PressButton(0x10, 9) == PressResult::InvalidButton);
}

BOOST_AUTO_TEST_CASE(Remotes_EmptyHub_ReturnsEmpty)
{
	SpasideRemoteController controller(hub);
	BOOST_CHECK_EQUAL(controller.Remotes().size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
