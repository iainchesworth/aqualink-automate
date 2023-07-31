#include <format>

#include "jandy/messages/serial_adapter/serial_adapter_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	SerialAdapterMessage::SerialAdapterMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	SerialAdapterMessage::~SerialAdapterMessage()
	{
	}

	std::string SerialAdapterMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
