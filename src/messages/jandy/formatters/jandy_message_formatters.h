#pragma once

#include <format>
#include <iostream>
#include <string>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message.h"
#include "messages/jandy/messages/jandy_message_ack.h"
#include "messages/jandy/messages/jandy_message_message.h"
#include "messages/jandy/messages/jandy_message_message_long.h"
#include "messages/jandy/messages/jandy_message_probe.h"
#include "messages/jandy/messages/jandy_message_status.h"
#include "messages/jandy/messages/jandy_message_unknown.h"


using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Messages::Jandy::Formatters

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage& obj);
std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyAckMessage& obj);
std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyMessageMessage& obj);
std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyMessageLongMessage& obj);
std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyProbeMessage& obj);
std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyStatusMessage& obj);
std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyUnknownMessage& obj);

template<std::derived_from<AqualinkAutomate::Messages::Jandy::Messages::JandyMessage> Derived, typename CharT>
struct std::formatter<Derived, CharT> : std::formatter<std::string>
{
	template<typename Context>
	auto format(Derived& msg, Context& context) const
	{
		return std::formatter<std::string>::format(msg.Print(), context);
	}
};
