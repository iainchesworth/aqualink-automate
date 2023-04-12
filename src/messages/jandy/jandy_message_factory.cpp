#include <memory>

#include "errors/protocol_error_codes.h"
#include "logging/logging.h"
#include "messages/jandy/jandy_message_constants.h"
#include "messages/jandy/jandy_message_factory.h"
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
			const auto message_type = message_bytes[1];
			std::shared_ptr<Messages::JandyMessage> message;

			switch (static_cast<JandyMessageTypes>(message_type))
			{
			case JandyMessageTypes::Probe:
				LogDebug(Channel::Messages, "Attempting to generate a Jandy Probe message type");
				message = std::make_shared<Messages::JandyProbeMessage>();
				break;

			case JandyMessageTypes::Ack:
				LogDebug(Channel::Messages, "Attempting to generate a Jandy Ack message type");
				message = std::make_shared<Messages::JandyAckMessage>();
				break;

			case JandyMessageTypes::Status:
				LogDebug(Channel::Messages, "Attempting to generate a Jandy Status message type");
				message = std::make_shared<Messages::JandyStatusMessage>();
				break;

			case JandyMessageTypes::Message:
				LogDebug(Channel::Messages, "Attempting to generate a Jandy Message message type");
				message = std::make_shared<Messages::JandyMessageMessage>();
				break;

			case JandyMessageTypes::MessageLong:
				LogDebug(Channel::Messages, "Attempting to generate a Jandy Message Long message type");
				message = std::make_shared<Messages::JandyMessageLongMessage>();
				break;

			case JandyMessageTypes::MessageLoopSt:
			case JandyMessageTypes::Unknown:
			default:
				LogDebug(Channel::Messages, "Attempting to generate a Jandy Unknown message type");
				message = std::make_shared<Messages::JandyUnknownMessage>();
				break;
			}

			LogDebug(Channel::Messages, "Attempting to deserialize serial bytes into generated message");
			message->Deserialize(message_bytes);
			return message;
		}

		return std::unexpected<boost::system::error_code>(return_value);
	}

}
// namespace AqualinkAutomate::Messages::Jandy
