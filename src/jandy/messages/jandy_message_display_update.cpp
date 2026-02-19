#include <format>

#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_display_update.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{
	JandyMessage_DisplayUpdate::JandyMessage_DisplayUpdate() noexcept :
		JandyMessage(JandyMessageIds::DisplayUpdate),
		Interfaces::IMessageSignalRecv<JandyMessage_DisplayUpdate>()
	{
	}

	JandyMessage_DisplayUpdate::~JandyMessage_DisplayUpdate() = default;

	std::string JandyMessage_DisplayUpdate::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_DisplayUpdate::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool JandyMessage_DisplayUpdate::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes into JandyMessage_DisplayUpdate type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
