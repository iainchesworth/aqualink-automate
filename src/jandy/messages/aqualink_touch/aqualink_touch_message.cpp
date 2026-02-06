#include <format>

#include "messages/aqualink_touch/aqualink_touch_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	AqualinkTouchMessage::AqualinkTouchMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	AqualinkTouchMessage::~AqualinkTouchMessage() = default;

	std::string AqualinkTouchMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

}
// namespace AqualinkAutomate::Messages
