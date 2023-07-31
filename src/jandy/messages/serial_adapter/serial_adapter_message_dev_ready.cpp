#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_ready.h"
#include "jandy/utility/jandy_checksum.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::SerialAdapterMessage_DevReady> SerialAdapterMessage_DevReady::g_SerialAdapterMessage_DevReady_Registration(JandyMessageIds::RSSA_DevReady);

	SerialAdapterMessage_DevReady::SerialAdapterMessage_DevReady() :
		SerialAdapterMessage(JandyMessageIds::RSSA_DevReady)
	{
	}

	SerialAdapterMessage_DevReady::~SerialAdapterMessage_DevReady()
	{
	}

	std::string SerialAdapterMessage_DevReady::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", SerialAdapterMessage::ToString(), 0);
	}

	bool SerialAdapterMessage_DevReady::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes =
		{
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_STX,
			0x00,
			magic_enum::enum_integer(JandyMessageIds::RSSA_DevReady),
			0x00,
			0x00,
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_ETX
		};

		auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
		message_bytes[5] = Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);

		return true;
	}

	bool SerialAdapterMessage_DevReady::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into SerialAdapterMessage_DevReady type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
