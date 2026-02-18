#pragma once

#include <memory>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "interfaces/icommanddispatcher.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/auxillary_devices/auxillary_device.h"

namespace AqualinkAutomate::Devices
{

	class IAQDevice;
	class SerialAdapterDevice;

	class CommandDispatcher : public Interfaces::ICommandDispatcher
	{
	public:
		CommandDispatcher(std::shared_ptr<Kernel::DataHub> data_hub, std::shared_ptr<Kernel::EquipmentHub> equipment_hub);

		CommandResult ToggleByUuid(const boost::uuids::uuid& uuid) override;
		CommandResult ToggleByLabel(const std::string& label) override;
		CommandResult CommandByUuid(const boost::uuids::uuid& uuid, DeviceAction action) override;
		CommandResult CommandByLabel(const std::string& label, DeviceAction action) override;
		CommandResult SetPoolSetpoint(uint8_t temperature) override;
		CommandResult SetSpaSetpoint(uint8_t temperature) override;
		CommandResult SetChlorinatorPercentage(uint8_t percentage) override;
		CommandResult SetChlorinatorBoost(bool enable) override;
		CommandResult SetCirculationMode(Kernel::CirculationModes mode) override;

	private:
		CommandResult DispatchCommand(const std::shared_ptr<Kernel::AuxillaryDevice>& device, DeviceAction action);
		SerialAdapterDevice* FindSerialAdapter();
		IAQDevice* FindIAQDevice();

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub;
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
	};

}
// namespace AqualinkAutomate::Devices
