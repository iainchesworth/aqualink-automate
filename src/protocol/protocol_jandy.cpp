#include "protocol/protocol_jandy.h"

namespace AqualinkAutomate::Protocol::Jandy
{

	Jandy::Jandy(std::shared_ptr<AqualinkAutomate::Serial::SerialPort> serial_port) :
		ProtocolHandler(serial_port)
	{
	}

	std::expected<std::shared_ptr<AqualinkAutomate::Messages::Message>, uint32_t> AqualinkAutomate::Protocol::Jandy::Jandy::GenerateMessageFromRawData()
	{
		return std::unexpected<uint32_t> { 0 };
	}

	void Jandy::HandleMessage(const AqualinkAutomate::Messages::Message& message)
	{

	}

	void Jandy::SendResponse(const AqualinkAutomate::Messages::Message& message)
	{

	}

}
// namespace AqualinkAutomate::Protocol::Jandy
