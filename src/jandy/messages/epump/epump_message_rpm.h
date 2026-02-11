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

	class EPumpMessage_RPM : public EPumpMessage, public Interfaces::IMessageSignalRecv<EPumpMessage_RPM>
	{
	public:
		static const uint8_t Index_RPM_Low = 6;
		static const uint8_t Index_RPM_High = 7;

	public:
		EPumpMessage_RPM() noexcept;
		virtual ~EPumpMessage_RPM();

	public:
		uint16_t RPM() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) override;

	private:
		uint16_t m_RPM;
	};

}
// namespace AqualinkAutomate::Messages
