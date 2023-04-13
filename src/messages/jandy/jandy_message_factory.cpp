#include <memory>

#include "errors/protocol_error_codes.h"
#include "logging/logging.h"
#include "messages/jandy/jandy_message_constants.h"
#include "messages/jandy/jandy_message_factory.h"
#include "messages/jandy/formatters/jandy_message_formatters.h"
#include "messages/jandy/messages/jandy_message_ack.h"
#include "messages/jandy/messages/jandy_message_message.h"
#include "messages/jandy/messages/jandy_message_message_long.h"
#include "messages/jandy/messages/jandy_message_probe.h"
#include "messages/jandy/messages/jandy_message_status.h"
#include "messages/jandy/messages/jandy_message_unknown.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::ErrorCodes;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy
{

	std::expected<JandyMessageGenerator::MessageType, boost::system::error_code> JandyMessageFactory::CreateFromSerialData(const std::span<const std::byte>& message_bytes)
	{
		boost::system::error_code return_value;

		if (2 > message_bytes.size())
		{
			LogDebug(Channel::Messages, "Attempted to generate a message from an invalid packet: data was too short");
			return_value = make_error_code(ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat);
		}
		else
		{
			const auto message_type = message_bytes[Messages::JandyMessage::Index_MessageType];
			std::shared_ptr<Messages::JandyMessage> message;

			switch (static_cast<JandyMessageTypes>(message_type))
			{
			case JandyMessageTypes::Probe:
				LogDebug(Channel::Messages, "Generating: Jandy Probe message");
				message = std::make_shared<Messages::JandyProbeMessage>();
				break;

			case JandyMessageTypes::Ack:
				LogDebug(Channel::Messages, "Generating: Jandy Ack message");
				message = std::make_shared<Messages::JandyAckMessage>();
				break;

			case JandyMessageTypes::Status:
				LogDebug(Channel::Messages, "Generating: Jandy Status message");
				message = std::make_shared<Messages::JandyStatusMessage>();
				break;

			case JandyMessageTypes::Message:
				LogDebug(Channel::Messages, "Generating: Jandy Message message");
				message = std::make_shared<Messages::JandyMessageMessage>();
				break;

			case JandyMessageTypes::MessageLong:
				LogDebug(Channel::Messages, "Generating: Jandy Message Long message");
				message = std::make_shared<Messages::JandyMessageLongMessage>();
				break;

			case JandyMessageTypes::MessageLoopStart:
			case JandyMessageTypes::Unknown:
			default:
				LogDebug(Channel::Messages, std::format("Generating: Jandy Unknown message (type: 0x{:02x})", static_cast<uint8_t>(message_type)));
				message = std::make_shared<Messages::JandyUnknownMessage>();
				break;
			}

			LogTrace(Channel::Messages, "Attempting to deserialize serial bytes into generated message");
			message->Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Message Contents -> {{{}}}", *message));
			return message;
		}

		return std::unexpected<boost::system::error_code>(return_value);
	}

}
// namespace AqualinkAutomate::Messages::Jandy
