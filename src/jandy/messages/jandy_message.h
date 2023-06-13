#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessage.h"
#include "interfaces/iserializable.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage : public Interfaces::IMessage<JandyMessageIds>, public Interfaces::ISerializable
	{
	public:
		static const uint8_t Index_DestinationId = 2;
		static const uint8_t Index_MessageType = 3;
		static const uint8_t Index_DataStart = 4;

	public:
		static const uint8_t PACKET_HEADER_LENGTH = 4;
		static const uint8_t PACKET_FOOTER_LENGTH = 3;
		static const uint8_t MAXIMUM_PACKET_LENGTH = 128;
		static const uint8_t MINIMUM_PACKET_LENGTH = PACKET_HEADER_LENGTH + PACKET_FOOTER_LENGTH;

	public:
		JandyMessage(const JandyMessageIds& msg_id);
		virtual ~JandyMessage();

	public:
		const Devices::JandyDeviceType Destination() const;
		const uint8_t RawId() const;
		const uint8_t MessageLength() const;
		const uint8_t ChecksumValue() const;

	public:
		virtual uint8_t MaxPermittedPacketLength() const override;
		virtual uint8_t MinPermittedPacketLength() const override;
		virtual std::string ToString() const override;

	public:
		virtual bool Serialize(std::vector<uint8_t>& message_bytes) const final;
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const = 0;
		virtual bool Deserialize(const std::span<const std::byte>& message_bytes) final;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) = 0;

	protected:
		bool PacketIsValid(const std::span<const std::byte>& message_bytes) const;

	protected:
		Devices::JandyDeviceType m_Destination;
		uint8_t m_RawId;
		uint8_t m_MessageLength;
		uint8_t m_ChecksumValue;
	};

}
// namespace AqualinkAutomate::Messages
