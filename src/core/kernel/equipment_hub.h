#pragma once

#include <memory>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

#include <boost/signals2.hpp>

#include <functional>

#include "interfaces/idevice.h"
#include "interfaces/ideviceidentifier.h"
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
		EquipmentHub() = default;
		~EquipmentHub() override = default;

		//---------------------------------------------------------------------
		// UPDATES / NOTIFICATIONS / EVENTS
		//---------------------------------------------------------------------

	public:
		mutable boost::signals2::signal<void(std::shared_ptr<Kernel::EquipmentHub_SystemEvent>)> EquipmentStatusChangeSignal;

		//---------------------------------------------------------------------
		// ACTIVE EQUIPMENT
		//---------------------------------------------------------------------

	public:
		bool EquipmentExists(std::unique_ptr<Interfaces::IEquipment> const& device) const;
		bool AddEquipment(std::unique_ptr<Interfaces::IEquipment> device);

		bool DeviceExists(const Interfaces::IDeviceIdentifier& device_id) const;
		bool DeviceExists(std::unique_ptr<Interfaces::IDevice> const& device) const;
		bool AddDevice(std::unique_ptr<Interfaces::IDevice> device);

		Interfaces::IDevice* FindDevice(std::function<bool(const Interfaces::IDevice&)> predicate) const;

		// NOTE: The equipment hub is intentionally unsynchronised. Device
		// registration (from the protocol task) and iteration (from the
		// HTTP/diagnostics handlers) both run on the single application thread
		// driven by the main poll() loop, so no locking is required. If a
		// multi-threaded execution model is ever reintroduced, this container
		// must be guarded before concurrent iteration/mutation.
		template<typename Func>
		void ForEachDevice(Func&& func) const
		{
			for (const auto& device : m_ActiveDevices)
			{
				func(*device);
			}
		}

	private:
		std::unordered_map<std::type_index, std::unique_ptr<Interfaces::IEquipment>> m_ActiveEquipment;
		std::unordered_set<std::unique_ptr<Interfaces::IDevice>> m_ActiveDevices;

	};

}
// namespace AqualinkAutomate::Equipment
