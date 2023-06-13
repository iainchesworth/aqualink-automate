#include <format>

#include "jandy/messages/pda/pda_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	PDAMessage::PDAMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	PDAMessage::~PDAMessage()
	{
	}

	std::string PDAMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
