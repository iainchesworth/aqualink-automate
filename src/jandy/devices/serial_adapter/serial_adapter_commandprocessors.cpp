#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/auxillaries/jandy_auxillary_traits_types.h"
#include "jandy/devices/serial_adapter_device.h"
#include "jandy/factories/jandy_auxillary_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{
	void SerialAdapterDevice::Command_SerialAdapter_Model(const uint16_t model_type)
	{
	}

	void SerialAdapterDevice::Command_SerialAdapter_OpMode(const Messages::SerialAdapter_SCS_OpModes& op_mode)
	{
		switch (op_mode)
		{
		case Messages::SerialAdapter_SCS_OpModes::Auto:
			JandyController::m_Config.Mode = Equipment::JandyEquipmentModes::Normal;
			break;

		case Messages::SerialAdapter_SCS_OpModes::Service:
			JandyController::m_Config.Mode = Equipment::JandyEquipmentModes::Service;
			break;

		case Messages::SerialAdapter_SCS_OpModes::Timeout:
			JandyController::m_Config.Mode = Equipment::JandyEquipmentModes::TimeOut;
			break; 

		case Messages::SerialAdapter_SCS_OpModes::Unknown:
		default:
			break;
		}
	}

	void SerialAdapterDevice::Command_SerialAdapter_Options(const Messages::SerialAdapter_SCS_Options& options)
	{
	}

	void SerialAdapterDevice::Command_SerialAdapter_BatteryCondition(const Messages::SerialAdapter_SCS_BatteryCondition& battery_condition)
	{
	}

	void SerialAdapterDevice::Command_SerialAdapter_AirTemperature(const Kernel::Temperature& temperature)
	{
		if (SERIALADAPTER_INVALID_TEMPERATURE_CUTOFF < temperature.InCelsius().value())
		{
			JandyController::m_Config.AirTemp(temperature);
		}
	}

	void SerialAdapterDevice::Command_SerialAdapter_PoolTemperature(const Kernel::Temperature& temperature)
	{
		if (SERIALADAPTER_INVALID_TEMPERATURE_CUTOFF < temperature.InCelsius().value())
		{
			JandyController::m_Config.PoolTemp(temperature);
		}
	}

	void SerialAdapterDevice::Command_SerialAdapter_SpaTemperature(const Kernel::Temperature& temperature)
	{
		if (SERIALADAPTER_INVALID_TEMPERATURE_CUTOFF < temperature.InCelsius().value())
		{
			JandyController::m_Config.SpaTemp(temperature);
		}
	}

	void SerialAdapterDevice::Command_SerialAdapter_SolarTemperature(const Kernel::Temperature& temperature)
	{
	}

	void SerialAdapterDevice::Command_SerialAdapter_AuxillaryStatus(const Auxillaries::JandyAuxillaryIds& aux_id, const Auxillaries::JandyAuxillaryStatuses& status)
	{
		std::shared_ptr<Kernel::AuxillaryDevice> aux_ptr(nullptr);

		//
		// Find (or create) a device that matches this auxillary.
		//

		auto auxillaries = JandyController::m_Config.Devices.FindByTrait(Auxillaries::JandyAuxillaryId{});

		auto aux_has_id = [&aux_id](auto& potential_match) -> bool
		{
			if (nullptr == potential_match)
			{
				return false;
			}
			else if (!potential_match->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}))
			{
				return false;
			}
			else
			{
				return (*(potential_match->AuxillaryTraits[Auxillaries::JandyAuxillaryId{}]) == aux_id);
			}
		};

		if (auto auxillary_it = std::find_if(auxillaries.begin(), auxillaries.end(), aux_has_id); auxillaries.end() != auxillary_it)
		{
			aux_ptr = *auxillary_it;
		}
		else if (auto new_aux_ptr = Factory::JandyAuxillaryFactory::Instance().SerialAdapterDevice_CreateDevice(aux_id, status); new_aux_ptr.has_value())
		{
			JandyController::m_Config.Devices.Add(new_aux_ptr.value());
			aux_ptr = new_aux_ptr.value();			
		}
		else
		{
			LogDebug(Channel::Devices, std::format("Failed to create auxillary device; error reported was: {}", new_aux_ptr.error().message()));
		}

		//
		// Update the auxillary's status
		//

		if (nullptr != aux_ptr)
		{
			switch (status)
			{
			case Auxillaries::JandyAuxillaryStatuses::Off:
				aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryStatusTrait{}, Kernel::AuxillaryStatuses::Off);
				break;

			case Auxillaries::JandyAuxillaryStatuses::On:
				aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryStatusTrait{}, Kernel::AuxillaryStatuses::On);
				break;

			case Auxillaries::JandyAuxillaryStatuses::Unknown:
				[[fallthrough]];
			default:
				aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryStatusTrait{}, Kernel::AuxillaryStatuses::Unknown);
				break;
			}
		}
	}

}
// namespace AqualinkAutomate::Devices
