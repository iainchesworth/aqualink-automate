#include <format>

#include "messages/aquarite/aquarite_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	AquariteMessage::AquariteMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}


	std::string AquariteMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
