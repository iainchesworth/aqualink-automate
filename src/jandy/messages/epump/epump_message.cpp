#include <format>

#include "messages/epump/epump_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	EPumpMessage::EPumpMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}


	std::string EPumpMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
