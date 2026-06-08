#pragma once

#include <memory>
#include <vector>

#include <boost/signals2.hpp>

#include "interfaces/iequipment.h"
#include "interfaces/istatuspublisher.h"
#include "devices/jandy_device.h"
#include "devices/jandy_device_id.h"
#include "generator/jandy_message_generator.h"
#include "messages/jandy_message.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"
#include "protocol/protocol_handler.h"

namespace AqualinkAutomate::Equipment
{

	class JandyEquipment : public Interfaces::IEquipment, public Interfaces::IStatusPublisher
	{
	public:
		JandyEquipment(Kernel::HubLocator& hub_locator, bool decode_to_master = false);
		~JandyEquipment() override;

	private:
		void IdentifyAndAddDevice(const Messages::JandyMessage& message);

	private:
		void DisplayUnknownMessages(const Messages::JandyMessage& message);

	private:
		std::vector<boost::signals2::connection> m_MessageConnections;

	private:
		Kernel::HubLocator& m_HubLocator;
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		// Raw (non-owning) pointer, NOT a shared_ptr: the EquipmentHub OWNS this
		// JandyEquipment (held by unique_ptr in the hub), so a shared_ptr back to
		// the hub forms a reference cycle that leaks the hub + all equipment and
		// devices at shutdown, and leaves this object's static message-signal
		// slots connected with a dangling HubLocator reference (use-after-free on
		// a later emission). The hub always outlives the equipment.
		Kernel::EquipmentHub* m_EquipmentHub{ nullptr };
		std::shared_ptr<Kernel::StatisticsHub> m_StatsHub{ nullptr };

	private:
		// --decode-to-master diagnostic: when true, frames addressed to the master (0x00)
		// are decoded and logged (observe-only). Off in normal operation.
		bool m_DecodeToMaster{ false };
	};

}
// namespace AqualinkAutomate::Equipment
