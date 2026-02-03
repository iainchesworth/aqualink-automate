#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/epump/epump_message.h"

namespace AqualinkAutomate::Messages
{

	class EPumpMessage_Watts : public EPumpMessage, public Interfaces::IMessageSignalRecv<EPumpMessage_Watts>
	{
	public:
		static const uint8_t Index_Watts_Low = 7;
		static const uint8_t Index_Watts_High = 8;

	public:
		EPumpMessage_Watts() noexcept;
		virtual ~EPumpMessage_Watts();

	public:
		uint16_t Watts() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		uint16_t m_Watts;
	};

}
// namespace AqualinkAutomate::Messages
