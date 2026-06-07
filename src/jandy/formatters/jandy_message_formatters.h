#pragma once

#include <concepts>
#include <format>
#include <ostream>
#include <string>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

#include "messages/jandy_message.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_message.h"
#include "messages/jandy_message_message_long.h"
#include "messages/jandy_message_probe.h"
#include "messages/jandy_message_status.h"
#include "messages/jandy_message_unknown.h"
#include "messages/aquarite/aquarite_message_getid.h"
#include "messages/aquarite/aquarite_message_percent.h"
#include "messages/aquarite/aquarite_message_ppm.h"

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
struct std::formatter<AqualinkAutomate::Messages::JandyMessageIds>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Messages::JandyMessageIds& msg_id, FormatContext& ctx) const
	{
		const auto v{ magic_enum::enum_name(msg_id) };
		return std::format_to(ctx.out(), "{}", v);
	}
};

template<>
struct std::formatter<AqualinkAutomate::Messages::JandyMessage>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Messages::JandyMessage& msg, FormatContext& ctx) const
	{
		const auto v{ msg.ToString() };
		return std::format_to(ctx.out(), "{}", v);
	}
};
