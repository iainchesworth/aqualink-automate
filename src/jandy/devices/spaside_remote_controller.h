#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "interfaces/ispasideremotecontroller.h"
#include "kernel/equipment_hub.h"

namespace AqualinkAutomate::Devices
{

	// Concrete spa-side remote control surface. Lives in the Jandy layer because it must
	// dynamic_cast the EquipmentHub's devices to the concrete SpasideRemoteDevice (core cannot
	// see Jandy types). Registered in the HubLocator as an Interfaces::ISpasideRemoteController so
	// the /api/equipment/spaside-remotes web route -- which only knows the core interface -- can
	// read decoded state and inject button presses on emulated remotes.
	class SpasideRemoteController : public Interfaces::ISpasideRemoteController
	{
	public:
		explicit SpasideRemoteController(std::shared_ptr<Kernel::EquipmentHub> equipment_hub);
		~SpasideRemoteController() override = default;

	public:
		std::vector<RemoteState> Remotes() const override;
		PressResult PressButton(uint8_t address, uint8_t button_index) override;

	private:
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
	};

}
// namespace AqualinkAutomate::Devices
