#include <format>

#include "messages/heater/heater_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	HeaterMessage::HeaterMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	HeaterMessage::~HeaterMessage() = default;

	std::string HeaterMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
