#pragma once

#include <cstdint>
#include <expected>
#include <span>

#include <boost/asio/awaitable.hpp>

#include "errors/error_codes.h"
#include "messages/jandy/jandy_message_type.h"
#include "messages/message_generator.h"

namespace AqualinkAutomate::Messages::Jandy
{

	class JandyMessageGenerator : public MessageGenerator<JandyMessageTypePtr, AqualinkAutomate::ErrorCodes::ErrorCode>
	{
	public:
		JandyMessageGenerator();

	public:
		virtual boost::asio::awaitable<std::expected<JandyMessageGenerator::MessageType, JandyMessageGenerator::ErrorType>> GenerateMessageFromRawData();
	};

}
// namespace AqualinkAutomate::Messages::Jandy
