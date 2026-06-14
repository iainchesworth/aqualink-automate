#include <format>
#include <string>

#include <magic_enum/magic_enum.hpp>

#include "devices/spaside_remote_controller.h"
#include "devices/spaside_remote_device.h"
#include "devices/capabilities/spa_switch_configurator.h"
#include "interfaces/idevice.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	SpasideRemoteController::SpasideRemoteController(std::shared_ptr<Kernel::EquipmentHub> equipment_hub) :
		m_EquipmentHub(std::move(equipment_hub))
	{
	}

	std::vector<Interfaces::ISpasideRemoteController::RemoteState> SpasideRemoteController::Remotes() const
	{
		std::vector<RemoteState> remotes;

		if (nullptr == m_EquipmentHub)
		{
			return remotes;
		}

		m_EquipmentHub->ForEachDevice([&remotes](Interfaces::IDevice& device)
		{
			auto* spaside = dynamic_cast<SpasideRemoteDevice*>(&device);
			if (nullptr == spaside)
			{
				return;
			}

			RemoteState state;
			state.address = spaside->DeviceId().Id()();
			state.device_class = std::string(magic_enum::enum_name(spaside->DeviceId().Class()));
			state.emulated = spaside->IsEmulated();
			state.button_count = spaside->ButtonCount();
			state.poll_count = spaside->PollCount();
			state.last_button = spaside->LastButton();
			state.led_image_seen = spaside->LedImageSeen();
			state.leds = spaside->LedStates();
			state.led_image = spaside->LedImageHex();

			remotes.push_back(std::move(state));
		});

		return remotes;
	}

	Interfaces::ISpasideRemoteController::PressResult SpasideRemoteController::PressButton(uint8_t address, uint8_t button_index)
	{
		if (nullptr == m_EquipmentHub)
		{
			return PressResult::RemoteNotFound;
		}

		SpasideRemoteDevice* target{ nullptr };
		m_EquipmentHub->ForEachDevice([&target, address](Interfaces::IDevice& device)
		{
			auto* spaside = dynamic_cast<SpasideRemoteDevice*>(&device);
			if ((nullptr != spaside) && (spaside->DeviceId().Id()() == address))
			{
				target = spaside;
			}
		});

		if (nullptr == target)
		{
			return PressResult::RemoteNotFound;
		}

		// Only an emulated remote can be actuated: for a passively-observed real remote WE are not
		// the transmitter (the press would be silently dropped by Capabilities::Emulated), so reject
		// it explicitly rather than pretend it worked.
		if (!target->IsEmulated())
		{
			return PressResult::NotEmulated;
		}

		// Buttons are 1..button_count (0 is the idle/release code, not a press).
		if ((button_index < 1) || (button_index > target->ButtonCount()))
		{
			return PressResult::InvalidButton;
		}

		LogInfo(Channel::Web, std::format("SpasideRemoteController: injecting press of button {} on emulated remote 0x{:02x}", button_index, address));
		target->PressButton(button_index);
		return PressResult::Success;
	}

	Interfaces::ISpasideRemoteController::AssignResult SpasideRemoteController::SetButtonAssignment(uint8_t switch_number, uint8_t button_number, const std::string& function)
	{
		// Programming is a CONTROLLER-config operation keyed by the controller's own switch
		// numbering (1..3) and button numbering (1..N), independent of which spa switches are
		// bus-attached -- so validate against those, not against any decoded remote.
		if ((switch_number < 1) || (switch_number > MAX_SWITCH_NUMBER) ||
			(button_number < 1) || (button_number > MAX_BUTTON_NUMBER) ||
			function.empty())
		{
			return AssignResult::InvalidRequest;
		}

		if (nullptr == m_EquipmentHub)
		{
			return AssignResult::NotAvailable;
		}

		// Find the highest-priority controller that can program assignments (OneTouch / iAQ),
		// mirroring CommandDispatcher::FindCapable: lowest ActuationPriority wins.
		Capabilities::SpaSwitchConfigurator* best{ nullptr };
		Capabilities::ActuationPriority best_priority{ Capabilities::ActuationPriority::Lowest };
		m_EquipmentHub->ForEachDevice([&best, &best_priority](Interfaces::IDevice& device)
		{
			auto* candidate = dynamic_cast<Capabilities::SpaSwitchConfigurator*>(&device);
			if (nullptr == candidate)
			{
				return;
			}
			const auto priority = candidate->ControllerPriority();
			if ((nullptr == best) || (static_cast<int>(priority) < static_cast<int>(best_priority)))
			{
				best = candidate;
				best_priority = priority;
			}
		});

		if (nullptr == best)
		{
			LogWarning(Channel::Web, std::format("SpasideRemoteController: no controller can program spa-switch assignments (switch {} button {} -> '{}')", switch_number, button_number, function));
			return AssignResult::NotAvailable;
		}

		LogInfo(Channel::Web, std::format("SpasideRemoteController: programming switch {} button {} -> '{}'", switch_number, button_number, function));
		switch (best->SetSpaSwitchAssignment(switch_number, button_number, function))
		{
		case Capabilities::ActuationResult::Accepted:      return AssignResult::Accepted;
		case Capabilities::ActuationResult::InvalidValue:  return AssignResult::InvalidRequest;
		case Capabilities::ActuationResult::MappingFailed: return AssignResult::InvalidRequest;
		case Capabilities::ActuationResult::NotSupported:
		default:                                           return AssignResult::NotAvailable;
		}
	}

}
// namespace AqualinkAutomate::Devices
