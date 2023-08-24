#pragma once

#include <memory>

#include <boost/signals2.hpp>

#include "interfaces/ihub.h"
#include "kernel/hub_events/equipment_hub_system_event.h"

namespace AqualinkAutomate::Kernel
{

	class EquipmentHub : public Interfaces::IHub
	{
	public:
		EquipmentHub();
		virtual ~EquipmentHub();

		//---------------------------------------------------------------------
		// UPDATES / NOTIFICATIONS / EVENTS
		//---------------------------------------------------------------------

	public:
		mutable boost::signals2::signal<void(std::shared_ptr<Kernel::EquipmentHub_SystemEvent>)> ServiceUpdateSignal;
	};

}
// namespace AqualinkAutomate::Equipment
