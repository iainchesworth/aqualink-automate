#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessage.h"
#include "interfaces/iserializable.h"
#include "jandy/messages/jandy_message_ids.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage : public Interfaces::IMessage<JandyMessageIds>, public Interfaces::ISerializable
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
		JandyMessage(const JandyMessageIds& msg_id);
		virtual ~JandyMessage();

	public:
		const uint8_t DestinationId() const;
		const uint8_t MessageType() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;

	protected:
		bool PacketIsValid(const std::span<const std::byte>& message_bytes) const;

	protected:
		uint8_t m_Destination;
		uint8_t m_MessageType;
	};

}
// namespace AqualinkAutomate::Messages
