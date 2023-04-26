#include <format>
#include <memory>
#include <typeinfo>
#include <utility>

#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/factories/jandy_message_factory.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/types/jandy_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Factory
{

	JandyMessageFactory::JandyMessageFactory() : 
		m_Generators{}
	{
	}

	JandyMessageFactory& JandyMessageFactory::Instance()
	{
		static JandyMessageFactory instance;
		return instance;
	}

	bool JandyMessageFactory::Register(const Messages::JandyMessageIds id, const JandyMessageInstanceGenerator generator)
	{
		return m_Generators.insert(std::make_pair(id, generator)).second;
	}

	std::expected<Types::JandyMessageTypePtr, Types::JandyErrorCode> JandyMessageFactory::CreateFromSerialData(const std::span<const std::byte>& message_bytes)
	{
		std::shared_ptr<Messages::JandyMessage> message{ nullptr };
		boost::system::error_code return_value;

		if (2 > message_bytes.size())
		{
			LogDebug(Channel::Messages, "Attempted to generate a message from an invalid packet: data was too short");
			return_value = make_error_code(ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat);
		}
		else
		{
			const auto message_type = static_cast<Messages::JandyMessageIds>(message_bytes[Messages::JandyMessage::Index_MessageType]);
			if (auto it = m_Generators.find(message_type); m_Generators.end() != it)
			{
				LogDebug(Channel::Messages, "Generating: Jandy message");
				message = it->second();
			}
			else if (it = m_Generators.find(Messages::JandyMessageIds::Unknown); m_Generators.end() != it)
			{
				LogDebug(Channel::Messages, std::format("Generating: Jandy Unknown message (type: 0x{:02x})", static_cast<uint8_t>(message_type)));
				message = it->second();
			}
			else
			{
				LogDebug(Channel::Messages, std::format("Both message type (0x{:02x}) and JandyMessage_Unknown generators missing from JandyMessageFactory", static_cast<uint8_t>(message_type)));
				return_value = make_error_code(ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator);
			}

			if (nullptr != message)
			{
				LogTrace(Channel::Messages, "Attempting to deserialize serial bytes into generated message");
				message->Deserialize(message_bytes);

				LogTrace(Channel::Messages, std::format("Message Contents -> {{{}}}", *message));
				return message;
			}
		}

		return std::unexpected<boost::system::error_code>(return_value);
	}

}
// namespace AqualinkAutomate::Factory
