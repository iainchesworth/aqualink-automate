#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "interfaces/iequipment.h"
#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/generator/jandy_rawdata_generator.h"
#include "jandy/messages/jandy_message.h"
#include "protocol/protocol_handler.h"

// Forward declarations
namespace AqualinkAutomate::HTTP
{
	class WebRoute_JandyEquipment;
	class WebRoute_JandyEquipment_Buttons;
	class WebRoute_JandyEquipment_Stats;
	class WebRoute_JandyEquipment_Version;
}
// namespace AqualinkAutomate::HTTP

namespace AqualinkAutomate::Equipment
{

	class JandyEquipment : public Interfaces::IEquipment
	{
		friend class AqualinkAutomate::HTTP::WebRoute_JandyEquipment;
		friend class AqualinkAutomate::HTTP::WebRoute_JandyEquipment_Buttons;
		friend class AqualinkAutomate::HTTP::WebRoute_JandyEquipment_Stats;
		friend class AqualinkAutomate::HTTP::WebRoute_JandyEquipment_Version;

	public:
		JandyEquipment(boost::asio::io_context& io_context, Config::JandyConfig& config);
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
		std::unordered_map<Messages::JandyMessageIds, uint32_t> m_MessageStats;
		Config::JandyConfig& m_Config;
	};

}
// namespace AqualinkAutomate::Equipment
