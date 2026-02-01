#include <format>

#include "messages/jandy_message_constants.h"
#include "messages/jandy_message_ids.h"
#include "messages/aqualink_touch/aqualink_touch_device_status.h"
#include "utility/jandy_checksum.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	AqualinkTouch_DeviceStatus::AqualinkTouch_DeviceStatus() noexcept :
		AqualinkTouchMessage(JandyMessageIds::AqualinkTouch_DeviceStatus),
		Interfaces::IMessageSignalRecv<AqualinkTouch_DeviceStatus>()
	{
	}

	AqualinkTouch_DeviceStatus::~AqualinkTouch_DeviceStatus()
	{
	}

	std::string AqualinkTouch_DeviceStatus::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", AqualinkTouchMessage::ToString(), 0);
	}

	bool AqualinkTouch_DeviceStatus::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes =
		{
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_STX,
			0x00,
			magic_enum::enum_integer(JandyMessageIds::AqualinkTouch_DeviceStatus),
			0x00,
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_ETX
		};

		auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
		message_bytes[5] = Utility::JandyPacket_CalculateChecksum(message_span_to_checksum.cbegin(), message_span_to_checksum.cend());

		return true;
	}

	bool AqualinkTouch_DeviceStatus::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AqualinkTouch_DeviceStatus type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
