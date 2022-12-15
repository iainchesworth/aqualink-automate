#pragma once

#include <boost/asio/spawn.hpp>

#include "messages/message.h"
#include "serial/serial_port.h"

namespace AqualinkAutomate::Protocol
{

	class ProtocolHandler
	{
	public:
		ProtocolHandler(AqualinkAutomate::Serial::SerialPort& serial_port);

	public:
		void Run(boost::asio::yield_context yield);

	private:
		virtual void HandleMessage(const AqualinkAutomate::Messages::Message& message, boost::asio::yield_context yield) = 0;
		virtual void SendResponse(const AqualinkAutomate::Messages::Message& message, boost::asio::yield_context yield) = 0;

	private:
		AqualinkAutomate::Serial::SerialPort& m_SerialPort;
	};

}
// namespace AqualinkAutomate::Protocol
