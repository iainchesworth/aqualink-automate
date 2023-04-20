#pragma once

#include <cstdint>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "interfaces/iequipment.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"
#include "jandy/messages/aquarite/aquarite_message_ppm.h"
#include "jandy/types/jandy_types.h"
#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Equipment
{
	class JandyEquipment : public Interfaces::IEquipment<Protocol::ProtocolHandler<Generators::JandyMessageGenerator>>
	{
	public:
		JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler);

	private:
		void Slot_AllMessageTypes(const Types::JandyMessageTypePtr msg);
		void Slot_Aquarite_GetId(const Messages::AquariteMessage_GetId& msg);
		void Slot_Aquarite_Percent(const Messages::AquariteMessage_Percent& msg);
		void Slot_Aquarite_PPM(const Messages::AquariteMessage_PPM& msg);

	private:
		auto IsDeviceRegistered(Interfaces::IDevice::DeviceId device_id);

	private:
		void StopAndCleanUp() override;

	private:
		boost::asio::io_context& m_IOContext;
		std::vector<std::shared_ptr<Interfaces::IDevice>> m_Devices;
		uint32_t m_MessagesProcessed = 0;
	};

}
// namespace AqualinkAutomate::Equipment
