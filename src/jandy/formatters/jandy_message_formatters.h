#pragma once

#include <concepts>
#include <format>
#include <iostream>
#include <string>
#include <type_traits>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"
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
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessageIds& obj);

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Ack& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Message& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_MessageLong& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Probe& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Status& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Unknown& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Messages::JandyMessageIds> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Messages::JandyMessageIds& msg_id, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(magic_enum::enum_name(msg_id)));
	}
};

template<>
struct std::formatter<AqualinkAutomate::Messages::JandyMessage> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Messages::JandyMessage& msg, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(msg.ToString()));
	}
};
