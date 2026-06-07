#include <memory>

#include "logging/logging.h"
#include "profiling/profiling.h"

#include "kernel/data_hub.h"
#include "devices/iaq_device.h"
#include "devices/keypad_device.h"
#include "devices/onetouch_device.h"
#include "devices/pda_device.h"
#include "devices/serial_adapter_device.h"
#include "options/options_jandy.h"
#include "equipment/jandy_equipment.h"
#include "jandy.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Jandy
{

	void Initialise(Kernel::HubLocator& hub_locator)
	{
	}

	void Configure(Kernel::HubLocator& hub_locator, const AqualinkAutomate::Options::Settings& settings)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Jandy::Initialise", std::source_location::current());

		auto jandy_settings_result = settings.Get<Jandy::Options::JandySettings>();
		if (!jandy_settings_result)
		{
			LogError(Channel::Main, "Failed to retrieve Jandy settings; Jandy equipment will not be configured");
			return;
		}

		const auto& jandy_settings = jandy_settings_result.value().get();

		auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
		if (nullptr == equipment_hub)
		{
			LogError(Channel::Main, "Failed to locate EquipmentHub; Jandy equipment will not be configured");
			return;
		}

		//---------------------------------------------------------------------
		// JANDY EQUIPMENT
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::JandyEquipment...");

		equipment_hub->AddEquipment(std::make_unique<Equipment::JandyEquipment>(hub_locator));

		if (jandy_settings.disable_emulation)
		{
			auto data_hub = hub_locator.Find<Kernel::DataHub>();
			if (nullptr != data_hub)
			{
				data_hub->EmulationDisabled = true;
				LogInfo(Channel::Main, "Emulation disabled; skipping equipment discovery phase");
			}
		}

		if (!jandy_settings.disable_emulation)
		{
			for (const auto& [controller_type, device_type] : jandy_settings.emulated_devices)
			{
				LogInfo(Channel::Main, std::format("Enabling controller emulation; type: {}, id: {}", magic_enum::enum_name(controller_type), device_type.Id()));

				auto device_id = std::make_shared<Devices::JandyDeviceType>(device_type);

				switch (controller_type)
				{
				case Devices::JandyEmulatedDeviceTypes::OneTouch:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::OneTouchDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::RS_Keypad:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::KeypadDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::IAQ:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::IAQDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::PDA:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::PDADevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::SerialAdapter:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::SerialAdapterDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::Unknown:
				default:
					LogWarning(Channel::Main, "Unknown emulated device type; cannot create controller device");
				}
			}
		}
	}

}
// namespace AqualinkAutomate::Jandy
