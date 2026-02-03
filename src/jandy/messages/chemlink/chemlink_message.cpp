#include <format>

#include "messages/chemlink/chemlink_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	ChemlinkMessage::ChemlinkMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	ChemlinkMessage::~ChemlinkMessage()
	{
	}

	std::string ChemlinkMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
