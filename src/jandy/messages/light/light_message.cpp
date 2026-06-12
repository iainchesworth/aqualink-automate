#include <format>

#include "messages/light/light_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	LightMessage::LightMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}


	std::string LightMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
