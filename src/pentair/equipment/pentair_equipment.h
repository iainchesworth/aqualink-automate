#pragma once

#include <vector>

#include <boost/signals2.hpp>

#include "interfaces/iequipment.h"
#include "interfaces/istatuspublisher.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"
#include "messages/chlorinator/pentair_chlorinator_message_status.h"
#include "messages/controller/pentair_controller_message_status.h"
#include "messages/pump/pentair_pump_message_status.h"

namespace AqualinkAutomate::Pentair::Equipment
{

	// Pentair equipment coordinator.  Mirrors JandyEquipment: it subscribes to
	// decoded Pentair messages and, on first sight of a device at a known address
	// range, instantiates the matching device handler and registers it with the
	// EquipmentHub (which then participates in further decoding via its slots).
	class PentairEquipment : public Interfaces::IEquipment, public Interfaces::IStatusPublisher
	{
	public:
		PentairEquipment(Kernel::HubLocator& hub_locator);
		~PentairEquipment() override;

	private:
		void IdentifyAndAddPump(const Messages::PentairPumpMessage_Status& message);
		void IdentifyAndAddChlorinator(const Messages::PentairChlorinatorMessage_Status& message);
		void IdentifyAndAddController(const Messages::PentairControllerMessage_Status& message);

	private:
		std::vector<boost::signals2::connection> m_MessageConnections;

	private:
		Kernel::HubLocator& m_HubLocator;
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub{ nullptr };
		std::shared_ptr<Kernel::StatisticsHub> m_StatsHub{ nullptr };
	};

}
// namespace AqualinkAutomate::Pentair::Equipment
