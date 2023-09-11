#pragma once

#include <memory>
#include <vector>

#include <boost/signals2.hpp>

#include "interfaces/idevice.h"
#include "interfaces/ideviceidentifier.h"
#include "interfaces/istatuspublisher.h"
#include "interfaces/iequipment.h"
#include "interfaces/ihub.h"
#include "kernel/hub_events/equipment_hub_system_event.h"
#include "kernel/hub_events/equipment_hub_system_event_status_change.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	struct EquipmentWithConnectedDevices
	{
		std::shared_ptr<Interfaces::IEquipment> Equipment;
		std::vector<std::unique_ptr<Interfaces::IDevice>> Devices;
	};

	class EquipmentHub : public Interfaces::IHub
	{
	public:
		EquipmentHub();
		virtual ~EquipmentHub();

		//---------------------------------------------------------------------
		// UPDATES / NOTIFICATIONS / EVENTS
		//---------------------------------------------------------------------

	public:
		mutable boost::signals2::signal<void(std::shared_ptr<Kernel::EquipmentHub_SystemEvent>)> EquipmentStatusChangeSignal;

		//---------------------------------------------------------------------
		// ACTIVE EQUIPMENT
		//---------------------------------------------------------------------

	public:
		const std::vector<std::shared_ptr<Interfaces::IEquipment>>& ActiveEquipment() const;
		const std::vector<std::shared_ptr<Interfaces::IDevice>>& ActiveDevices() const;

	public:
		void AddEquipment(std::shared_ptr<Interfaces::IEquipment> equipment);
		void AddDevice(std::shared_ptr<Interfaces::IDevice> device);

	public:
		bool IsEquipmentRegistered(const std::shared_ptr<Interfaces::IEquipment> equipment) const;
		bool IsDeviceRegistered(const Interfaces::IDeviceIdentifier& device_id) const;

	private:
		std::vector<std::shared_ptr<Interfaces::IEquipment>> m_ActiveEquipment{};
		std::vector<std::shared_ptr<Interfaces::IDevice>> m_ActiveDevices{};

		//---------------------------------------------------------------------
		// SERVICE STATUS
		//---------------------------------------------------------------------

	public:

	private:
		template<typename EQUIPMENT_TYPE>
		void CheckAndRegisterForUpdateEvents(const std::shared_ptr<EQUIPMENT_TYPE> status_publisher_ptr)
		{
			if (auto ptr = std::dynamic_pointer_cast<Interfaces::IStatusPublisher>(status_publisher_ptr); nullptr == ptr)
			{
				LogTrace(Channel::Equipment, "IStatusPublisher interface not supported; will not attempt to chain status signals");
			}
			else
			{
				ptr->StatusSignal.connect(
					[this](Interfaces::IStatusPublisher::StatusType status) -> void
					{
						if (auto status_ptr = status.lock(); nullptr == status_ptr)
						{
							LogDebug(Channel::Equipment, "Status was unavailable when locking was attempted; ignoring status change");
						}
						else
						{
							LogTrace(Channel::Equipment, "Publishing a status change has occurred for a connected equipment/device");
							EquipmentStatusChangeSignal(std::make_shared<Kernel::EquipmentHub_SystemEvent_StatusChange>(*status_ptr));
						}
					}
				);
			}
		}
	};

}
// namespace AqualinkAutomate::Equipment
