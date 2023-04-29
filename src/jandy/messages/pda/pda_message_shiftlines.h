#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/pda/pda_message.h"

namespace AqualinkAutomate::Messages
{

	class PDAMessage_ShiftLines : public PDAMessage, public Interfaces::IMessageSignal<PDAMessage_ShiftLines>
	{
	public:
		static const uint8_t Index_FirstLineId = 4;
		static const uint8_t Index_LastLineId = 5;
		static const uint8_t Index_LineShift = 6;

	public:
		PDAMessage_ShiftLines();
		virtual ~PDAMessage_ShiftLines();

	public:
		uint8_t FirstLineId() const;
		uint8_t LastLineId() const;
		int8_t LineShift() const;

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		uint8_t m_FirstLineId;
		uint8_t m_LastLineId;
		int8_t m_LineShift;

	private:
		static const Factory::JandyMessageRegistration<Messages::PDAMessage_ShiftLines> g_PDAMessage_ShiftLines_Registration;
	};

}
// namespace AqualinkAutomate::Messages
