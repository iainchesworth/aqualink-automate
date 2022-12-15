#pragma once

#include <boost/asio/spawn.hpp>

#include "messages/message.h"
#include "protocol/protocol_handler.h"
#include "serial/serial_port.h"

namespace AqualinkAutomate::Protocol::Jandy
{

	class Jandy : public AqualinkAutomate::Protocol::ProtocolHandler
	{
	public:
		Jandy(AqualinkAutomate::Serial::SerialPort& serial_port);

	public:
		void HandleMessage(const AqualinkAutomate::Messages::Message& message, boost::asio::yield_context yield);
		void SendResponse(const AqualinkAutomate::Messages::Message& message, boost::asio::yield_context yield);

	};

}
// namespace AqualinkAutomate::Protocol::Jandy
