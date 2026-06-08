#include <format>

#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_unknown.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{
	JandyMessage_Unknown::JandyMessage_Unknown() noexcept :
		JandyMessage(JandyMessageIds::Unknown),
		Interfaces::IMessageSignalRecv<JandyMessage_Unknown>()
	{
	}


	std::string JandyMessage_Unknown::ToString() const
	{
		std::string payload_hex;
		payload_hex.reserve(m_Payload.size() * 3);
		for (const auto byte : m_Payload)
		{
			payload_hex += std::format("{:02x} ", byte);
		}
		if (!payload_hex.empty())
		{
			payload_hex.pop_back(); // drop trailing space
		}

		return std::format("Packet: {} || Payload ({} bytes){}{}", JandyMessage::ToString(), m_Payload.size(), m_Payload.empty() ? "" : ": ", payload_hex);
	}

	const std::vector<uint8_t>& JandyMessage_Unknown::Payload() const
	{
		return m_Payload;
	}

	bool JandyMessage_Unknown::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		LogTrace(Channel::Messages, std::format("Serialising JandyMessage_Unknown type into {} bytes", message_bytes.size()));

		// Emit the retained payload so a round-tripped Unknown frame keeps its bytes
		// (previously the payload was silently dropped on re-serialisation).
		message_bytes.insert(message_bytes.end(), m_Payload.begin(), m_Payload.end());

		return true;
	}

	bool JandyMessage_Unknown::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes into JandyMessage_Unknown type", message_bytes.size()));

		// Retain the raw payload verbatim (the bytes between the 4-byte header and the
		// 3-byte footer) so undecoded frames -- e.g. an unrecognised command sent TO the
		// master -- can still be inspected byte-for-byte by the to-master traffic decoder.
		m_Payload.clear();
		if (message_bytes.size() >= static_cast<std::size_t>(Index_DataStart) + PACKET_FOOTER_LENGTH)
		{
			const auto payload = message_bytes.subspan(Index_DataStart, message_bytes.size() - Index_DataStart - PACKET_FOOTER_LENGTH);
			m_Payload.assign(payload.begin(), payload.end());
		}

		return true;
	}

}
// namespace AqualinkAutomate::Messages
