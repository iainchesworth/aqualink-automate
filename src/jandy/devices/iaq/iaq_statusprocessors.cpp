#include <format>
#include <memory>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "devices/iaq_device.h"
#include "auxillaries/jandy_auxillary_id.h"
#include "auxillaries/jandy_auxillary_status.h"
#include "auxillaries/jandy_auxillary_traits_types.h"
#include "factories/jandy_auxillary_factory.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/hub_events/data_hub_config_event_button_state_change.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices
{

	void IAQDevice::ProcessMainStatus(const Messages::IAQMessage_MainStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::ProcessMainStatus", std::source_location::current());

		LogDebug(Channel::Devices, std::format("IAQ ({}): Processing MainStatus: pool={:.0f}F, spa={:.0f}F, air={:.0f}F, pump={}, pool_heat={}, spa_heat={}, solar={}",
			DeviceId(),
			msg.PoolTemperature().InFahrenheit().value(),
			msg.SpaTemperature().InFahrenheit().value(),
			msg.AirTemperature().InFahrenheit().value(),
			msg.PumpOn(),
			magic_enum::enum_name(msg.PoolHeaterStatus()),
			magic_enum::enum_name(msg.SpaHeaterStatus()),
			magic_enum::enum_name(msg.SolarHeaterStatus())));

		// Update temperatures in the DataHub.
		m_DataHub->PoolTemp(msg.PoolTemperature());
		m_DataHub->SpaTemp(msg.SpaTemperature());
		m_DataHub->AirTemp(msg.AirTemperature());

		// Update heater setpoint if present.
		if (auto setpoint = msg.HeaterSetpoint(); setpoint.has_value())
		{
			if (msg.SpaMode())
			{
				m_DataHub->SpaTempSetpoint(setpoint.value());
			}
			else
			{
				m_DataHub->PoolTempSetpoint(setpoint.value());
			}
		}

		// Update filter pump status.
		{
			if (0 == m_DataHub->FilterPumps().size())
			{
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Pump);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, Kernel::AuxillaryTraitsTypes::LabelTrait::COMMON_LABEL_FILTER_PUMP);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpSpeedTrait{}, Kernel::PumpSpeeds::Unknown);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpStatusTrait{}, Kernel::PumpStatuses::Unknown);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpTypeTrait{}, Kernel::PumpTypes::FilterCirculation);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			const auto pump_status = msg.PumpOn() ? Kernel::PumpStatuses::Running : Kernel::PumpStatuses::Off;
			for (auto& pump : m_DataHub->FilterPumps())
			{
				pump->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpStatusTrait{}, pump_status);

				auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(pump);
				auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(pump->Id(), status_string);
				m_DataHub->ConfigUpdateSignal(update_event);
			}
		}

		// Helper to create/update a heater device in the DataHub.
		auto update_heater = [this](const std::string& label, Kernel::HeaterStatuses status)
		{
			if (0 == m_DataHub->Devices.FindByLabel(label).size())
			{
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Heater);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::HeaterStatuses::Off);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			auto heater = m_DataHub->Devices.FindByLabel(label).front();
			heater->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}, status);

			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(heater);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(heater->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);
		};

		update_heater("Pool Heat", msg.PoolHeaterStatus());
		update_heater("Spa Heat", msg.SpaHeaterStatus());
		update_heater("Solar Heat", msg.SolarHeaterStatus());
	}

	void IAQDevice::ProcessAuxStatus(const Messages::IAQMessage_AuxStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::ProcessAuxStatus", std::source_location::current());

		LogDebug(Channel::Devices, std::format("IAQ ({}): Processing AuxStatus: {} devices", DeviceId(), msg.DeviceCount()));

		for (const auto& info : msg.Devices())
		{
			auto aux_id = magic_enum::enum_cast<Auxillaries::JandyAuxillaryIds>(info.device_index);
			if (!aux_id.has_value())
			{
				LogDebug(Channel::Devices, std::format("IAQ ({}): Unknown device_index {} in AuxStatus, skipping", DeviceId(), info.device_index));
				continue;
			}

			const auto status = info.is_on ? Auxillaries::JandyAuxillaryStatuses::On : Auxillaries::JandyAuxillaryStatuses::Off;

			// Find or create the auxillary device.
			std::shared_ptr<Kernel::AuxillaryDevice> aux_ptr(nullptr);
			auto auxillaries = m_DataHub->Devices.FindByTrait(Auxillaries::JandyAuxillaryId{});

			auto aux_has_id = [&aux_id](auto& potential_match) -> bool
			{
				if (nullptr == potential_match || !potential_match->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}))
				{
					return false;
				}
				return (*(potential_match->AuxillaryTraits[Auxillaries::JandyAuxillaryId{}]) == aux_id.value());
			};

			if (auto auxillary_it = std::find_if(auxillaries.begin(), auxillaries.end(), aux_has_id); auxillaries.end() != auxillary_it)
			{
				aux_ptr = *auxillary_it;
			}
			else if (auto new_aux_ptr = Factory::JandyAuxillaryFactory::Instance().SerialAdapterDevice_CreateDevice(aux_id.value(), status); new_aux_ptr.has_value())
			{
				m_DataHub->Devices.Add(new_aux_ptr.value());
				aux_ptr = new_aux_ptr.value();
			}
			else
			{
				LogDebug(Channel::Devices, std::format("IAQ ({}): Failed to create auxillary device for {}: {}", DeviceId(), magic_enum::enum_name(aux_id.value()), new_aux_ptr.error().message()));
				continue;
			}

			// Update the device status.
			aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryStatusTrait{},
				info.is_on ? Kernel::AuxillaryStatuses::On : Kernel::AuxillaryStatuses::Off);

			// Update the label from the IAQ-provided name if non-empty.
			if (!info.name.empty())
			{
				aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, info.name);
			}

			// Signal that a button state change has occurred.
			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(aux_ptr);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(aux_ptr->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);

			LogTrace(Channel::Devices, std::format("IAQ ({}): AuxStatus device {}: name='{}', status={}",
				DeviceId(), magic_enum::enum_name(aux_id.value()), info.name, info.is_on ? "On" : "Off"));
		}
	}

}
// namespace AqualinkAutomate::Devices
