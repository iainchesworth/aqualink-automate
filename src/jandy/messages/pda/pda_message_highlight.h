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

	class PDAMessage_Highlight : public PDAMessage, public Interfaces::IMessageSignalRecv<PDAMessage_Highlight>
	{
	public:
		static const uint8_t Index_LineId = 4;

	public:
		PDAMessage_Highlight();
		PDAMessage_Highlight(const uint8_t line_id);
		virtual ~PDAMessage_Highlight();

	public:
		uint8_t LineId() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		uint8_t m_LineId;

	private:
		static const Factory::JandyMessageRegistration<Messages::PDAMessage_Highlight> g_PDAMessage_Highlight_Registration;
	};

}
// namespace AqualinkAutomate::Messages
