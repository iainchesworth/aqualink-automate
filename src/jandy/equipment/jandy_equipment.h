#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "interfaces/iequipment.h"
#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/generator/jandy_rawdata_generator.h"
#include "jandy/messages/jandy_message.h"
#include "kernel/data_hub.h"
#include "kernel/statistics_hub.h"
#include "protocol/protocol_handler.h"

namespace AqualinkAutomate::Equipment
{

	class JandyEquipment : public Interfaces::IEquipment
	{
	public:
		JandyEquipment(boost::asio::io_context& io_context, Kernel::DataHub& data_hub, Kernel::StatisticsHub& statistics_hub);
		virtual ~JandyEquipment();

	private:
		auto IsDeviceRegistered(const Devices::JandyDeviceType& device_id);
		void IdentifyAndAddDevice(const Messages::JandyMessage& message);

	private:
		void DisplayUnknownMessages(const Messages::JandyMessage& message);

	public:
		bool AddEmulatedDevice(std::unique_ptr<Devices::JandyDevice> device);

	private:
		boost::asio::io_context& m_IOContext;
		std::vector<std::unique_ptr<Devices::JandyDevice>> m_Devices;
		std::unordered_set<Devices::JandyDeviceId> m_IdentifiedDeviceIds;

	private:
		Kernel::DataHub& m_DataHub;
		Kernel::StatisticsHub& m_StatsHub;
	};

}
// namespace AqualinkAutomate::Equipment
