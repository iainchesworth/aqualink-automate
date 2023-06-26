#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/pda/pda_message.h"

namespace AqualinkAutomate::Messages
{

	class PDAMessage_ShiftLines : public PDAMessage, public Interfaces::IMessageSignalRecv<PDAMessage_ShiftLines>
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
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		uint8_t m_FirstLineId;
		uint8_t m_LastLineId;
		int8_t m_LineShift;

	private:
		static const Factory::JandyMessageRegistration<Messages::PDAMessage_ShiftLines> g_PDAMessage_ShiftLines_Registration;
	};

}
// namespace AqualinkAutomate::Messages
