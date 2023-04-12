#pragma once

#include "equipment/equipment_base.h"
#include "messages/jandy/jandy_message_generator.h"
#include "messages/jandy/jandy_message_handler.h"
#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Protocol;

namespace AqualinkAutomate::Equipment::Jandy
{
	class JandyEquipment : public EquipmentBase<Messages::Jandy::JandyMessageHandler, Protocol::ProtocolHandler<Messages::Jandy::JandyMessageGenerator>>
	{
	public:
		JandyEquipment(Serial::SerialPort& serial_port);
	};

}
// namespace AqualinkAutomate::Equipment::Jandy
