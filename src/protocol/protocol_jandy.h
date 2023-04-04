#pragma once

#include <memory>
#include <utility>

#include "messages/message.h"
#include "protocol/protocol_handler.h"
#include "serial/serial_port.h"

namespace AqualinkAutomate::Protocol::Jandy
{
	struct MessageGenerator_Jandy
	{
	public:
		// discard data until header

		// get length and wait for that

		// extract a message and discard processed data

		// error and go back to start
	};


	class Jandy : public AqualinkAutomate::Protocol::ProtocolHandler
	{
	public:
		Jandy(std::shared_ptr<AqualinkAutomate::Serial::SerialPort> serial_port);

	public:
		std::expected<std::shared_ptr<AqualinkAutomate::Messages::Message>, uint32_t> GenerateMessageFromRawData();

	public:
		void HandleMessage(const AqualinkAutomate::Messages::Message& message);
		void SendResponse(const AqualinkAutomate::Messages::Message& message);

	};

}
// namespace AqualinkAutomate::Protocol::Jandy
