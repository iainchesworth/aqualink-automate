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

	class CommandDispatcher : public Interfaces::ICommandDispatcher
	{
	public:
		CommandDispatcher(std::shared_ptr<Kernel::DataHub> data_hub, std::shared_ptr<Kernel::EquipmentHub> equipment_hub);

		CommandResult ToggleByUuid(const boost::uuids::uuid& uuid) override;
		CommandResult ToggleByLabel(const std::string& label) override;

	private:
		CommandResult DispatchToggle(const std::shared_ptr<Kernel::AuxillaryDevice>& device);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub;
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
	};

}
// namespace AqualinkAutomate::Devices
