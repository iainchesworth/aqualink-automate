#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/signals2.hpp>

#include "interfaces/iequipment.h"
#include "interfaces/istatuspublisher.h"
#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/generator/jandy_rawdata_generator.h"
#include "jandy/messages/jandy_message.h"
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
		JandyEquipment(boost::asio::io_context& io_context, Kernel::HubLocator& hub_locator);
		virtual ~JandyEquipment();

	private:
		void IdentifyAndAddDevice(const Messages::JandyMessage& message);

	private:
		void DisplayUnknownMessages(const Messages::JandyMessage& message);

	private:
		std::unordered_set<Devices::JandyDeviceId> m_IdentifiedDeviceIds;

	private:
		std::vector<boost::signals2::connection> m_MessageConnections;

	private:
		Kernel::HubLocator& m_HubLocator;
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub{ nullptr };
		std::shared_ptr<Kernel::StatisticsHub> m_StatsHub{ nullptr };
	};

}
// namespace AqualinkAutomate::Equipment
