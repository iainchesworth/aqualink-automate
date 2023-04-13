#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>

#include "messages/message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	class JandyMessage : public Message
	{
		static const uint8_t DefaultDestination = 0x00;

	public:
		static const uint8_t Index_DestinationId = 2;
		static const uint8_t Index_MessageType = 3;
		static const uint8_t Index_DataStart = 4;

	public:
		static const uint8_t MAXIMUM_PACKET_LENGTH = 128;
		static const uint8_t MINIMUM_PACKET_LENGTH = 7;

	public:
		JandyMessage();
		virtual ~JandyMessage();

	public:
		const uint8_t DestinationId() const;

	public:
		virtual std::string Print() const override;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;

	protected:
		uint8_t m_Destination;
	};

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
