#pragma once

#include <format>
#include <iostream>
#include <string>

#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_message.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"
#include "jandy/messages/aquarite/aquarite_message_ppm.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std 
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Ack& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Message& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_MessageLong& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Probe& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Status& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Unknown& obj);

}
// namespace std

template<std::derived_from<AqualinkAutomate::Messages::JandyMessage> Derived, typename CharT>
struct std::formatter<Derived, CharT> : std::formatter<std::string>
{
	template<typename Context>
	auto format(Derived& msg, Context& context) const
	{
		return std::formatter<std::string>::format(msg.ToString(), context);
	}
};
