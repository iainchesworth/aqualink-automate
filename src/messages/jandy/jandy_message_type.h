#pragma once

#include <memory>

#include "messages/jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages::Jandy
{

	using JandyMessageType = AqualinkAutomate::Messages::Jandy::Messages::JandyMessage;
	using JandyMessageTypePtr = std::shared_ptr<JandyMessageType>;

}
// namespace AqualinkAutomate::Messages::Jandy
