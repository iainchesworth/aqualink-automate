#pragma once

#include <cstdint>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "interfaces/iequipment.h"
#include "jandy/bridge/jandy_message_bridge.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/types/jandy_types.h"
#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Equipment
{
	class JandyEquipment : public Interfaces::IEquipment<Protocol::ProtocolHandler<Generators::JandyMessageGenerator, Bridges::Bridge_JandyMessages>, Bridges::Bridge_JandyMessages>
	{
	public:
		JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler, MessageBridge& message_bridge);

	private:
		void Slot_AllMessageTypes(const Types::JandyMessageTypePtr& msg);
		void Slot_Aquarite_GetId(const std::shared_ptr<Messages::AquariteMessage_GetId>& msg);
		void Slot_Aquarite_Percent(const std::shared_ptr<Messages::AquariteMessage_Percent>& msg);
		void Slot_Aquarite_PPM(const std::shared_ptr<Messages::AquariteMessage_PPM>& msg);

	private:
		auto IsDeviceRegistered(Interfaces::IDevice::DeviceId device_id);

	private:
		boost::asio::io_context& m_IOContext;
		std::vector<std::shared_ptr<Interfaces::IDevice>> m_Devices;
		uint32_t m_MessagesProcessed = 0;
	};

}
// namespace AqualinkAutomate::Equipment
