#pragma once

#include <boost/asio/io_context.hpp>

#include "interfaces/iequipment.h"
#include "logging/logging.h"
#include "jandy/bridge/jandy_message_bridge.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/types/jandy_types.h"
#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Equipment
{
	class JandyEquipment : public Interfaces::IEquipment<Protocol::ProtocolHandler<Generators::JandyMessageGenerator>, Bridges::Bridge_JandyMessages>
	{
	public:
		JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler, MessageBridge& message_bridge);

	private:
		void Slot_AllMessageTypes(const Types::JandyMessageTypePtr& msg)
		{
			LogDebug(Logging::Channel::Main, "STUFF SIGNAL -> SLOT");
		}
	};

}
// namespace AqualinkAutomate::Equipment
