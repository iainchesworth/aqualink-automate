#include <algorithm>
#include <execution>

#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"
#include "kernel/data_hub_event_chemistry.h"
#include "kernel/data_hub_event_temperature.h"
#include "utility/case_insensitive_comparision.h"

namespace AqualinkAutomate::Kernel
{
	
	DataHub::DataHub()
	{
	}

	Kernel::Temperature DataHub::AirTemp() const
	{
		return m_AirTemp;
	}

	Kernel::Temperature DataHub::PoolTemp() const
	{
		return m_PoolTemp;
	}

	Kernel::Temperature DataHub::SpaTemp() const
	{
		return m_SpaTemp;
	}

	Kernel::Temperature DataHub::FreezeProtectPoint() const
	{
		return m_FreezeProtectPoint;
	}

	void DataHub::AirTemp(const Kernel::Temperature& air_temp)
	{
		m_AirTemp = air_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->AirTemp(m_AirTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::PoolTemp(const Kernel::Temperature& pool_temp)
	{
		m_PoolTemp = pool_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->PoolTemp(m_PoolTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SpaTemp(const Kernel::Temperature& spa_temp)
	{
		m_SpaTemp = spa_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->SpaTemp(m_SpaTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::FreezeProtectPoint(const Kernel::Temperature& freeze_protect_point)
	{
		m_FreezeProtectPoint = freeze_protect_point;
	}

	Kernel::ORP DataHub::ORP() const
	{
		return m_ORP;
	}

	Kernel::pH DataHub::pH() const
	{
		return m_pH;
	}

	ppm_quantity DataHub::SaltLevel() const
	{
		return m_SaltLevel;
	}

	void DataHub::ORP(const Kernel::ORP& orp)
	{
		m_ORP = orp;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Chemistry>();
		update_event->ORP(m_ORP);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::pH(const Kernel::pH& pH)
	{
		m_pH = pH;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Chemistry>();
		update_event->pH(m_pH);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SaltLevel(const ppm_quantity& salt_level_in_ppm)
	{
		m_SaltLevel = salt_level_in_ppm;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Chemistry>();
		update_event->SaltLevel(m_SaltLevel);
		ConfigUpdateSignal(update_event);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Auxillaries() const
	{
		using DeviceType = decltype(Auxillaries())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Chlorinators() const
	{
		using DeviceType = decltype(Chlorinators())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Heaters() const
	{
		using DeviceType = decltype(Heaters())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Heater);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Pumps() const
	{
		using DeviceType = decltype(Pumps())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Pump);
	}

	std::optional<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::FilterPump()
	{
		static std::shared_ptr<Kernel::AuxillaryDevice> filter_pump(nullptr);
		const std::string filter_pump_label{ "filter pump" };

		if (nullptr != filter_pump)
		{
			// Already know which device is the filter pump so use the cache.
			return filter_pump;
		}
		else 
		{
			for (auto base_ptr : Devices.FindByLabel(filter_pump_label))
			{
				if (nullptr != base_ptr)
				{
					// If there's at least one matching pump, use the first one found...
					filter_pump = base_ptr;
					return filter_pump;
				}
			}
		}

		return std::nullopt;
	}

}
// namespace AqualinkAutomate::Kernel
