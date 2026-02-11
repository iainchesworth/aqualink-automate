#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/chemlink/chemlink_message.h"

namespace AqualinkAutomate::Messages
{

	enum class ChemlinkDataTag : uint8_t
	{
		ORP = 0x02,
		pH = 0x03,
		pHFeeder = 0x08,
		ORPFeeder = 0x18,
		Unknown = 0xFF
	};

	class ChemlinkMessage_Response : public ChemlinkMessage, public Interfaces::IMessageSignalRecv<ChemlinkMessage_Response>
	{
	public:
		static const uint8_t Index_DataTag = 4;
		static const uint8_t Index_ValueLow = 5;
		static const uint8_t Index_ValueHigh = 6;

	public:
		ChemlinkMessage_Response() noexcept;
		virtual ~ChemlinkMessage_Response();

	public:
		ChemlinkDataTag DataTag() const;
		uint16_t RawValue() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) override;

	private:
		ChemlinkDataTag m_DataTag;
		uint16_t m_RawValue;
	};

}
// namespace AqualinkAutomate::Messages
