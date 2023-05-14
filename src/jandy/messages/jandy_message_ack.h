#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "interfaces/imessagesignal_send.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/publisher/jandy_message_publisher.h"

namespace AqualinkAutomate::Messages
{

	enum class AckTypes : uint8_t
	{
		ACK_IAQTouch = 0x00,
		ACK_PDA = 0x40,		

		// First Generation responses.
		V1_Normal = 0x00,
		V1_ScreenBusy_Scroll = 0x01,
		V1_ScreenBusy_Block = 0x03,

		// Second Generation responses.
		V2_Normal = 0x80,
		V2_ScreenBusy_Scroll = 0x81,
		V2_ScreenBusy_Block = 0x83,

		Unknown_3F = 0x3F,
		Unknown_70 = 0x70,
		Unknown_72 = 0x72,
		Unknown = 0xFF
	};

	class JandyMessage_Ack : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_Ack>, public Interfaces::IMessageSignalSend<JandyMessage_Ack, Publishers::JandyMessagePublisher>
	{
		static const uint8_t AQUALINK_MASTER_ID = 0x00;

		static const uint8_t Index_AckType = 4;
		static const uint8_t Index_Command = 5;

	public:
		JandyMessage_Ack();
		JandyMessage_Ack(AckTypes ack_type, uint8_t command);
		virtual ~JandyMessage_Ack();

	public:
		AckTypes AckType() const;
		uint8_t Command() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual void Serialize(std::vector<uint8_t>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;

	private:
		AckTypes m_AckType;
		uint8_t m_Command;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Ack> g_JandyMessage_Ack_Registration;
	};

}
// namespace AqualinkAutomate::Messages
