#include <format>

#include "jandy/messages/iaq/iaq_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage::IAQMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	IAQMessage::~IAQMessage()
	{
	}

	std::string IAQMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
