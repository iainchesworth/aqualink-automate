#include <boost/bind/bind.hpp>

#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::Equipment
{
	JandyEquipment::JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler, MessageBridge& message_bridge) :
		IEquipment(io_context, protocol_handler, message_bridge)
	{
		m_MessageBridge.Signal_AllJandyMessages.connect(boost::bind(&JandyEquipment::Slot_AllMessageTypes, this, boost::placeholders::_1));
	}
}
// namespace AqualinkAutomate::Equipment
