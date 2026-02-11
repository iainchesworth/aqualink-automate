#include <format>

#include <magic_enum/magic_enum.hpp>

#include "messages/chemlink/chemlink_message_response.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	ChemlinkMessage_Response::ChemlinkMessage_Response() noexcept :
		ChemlinkMessage(JandyMessageIds::Chemlink_Response),
		Interfaces::IMessageSignalRecv<ChemlinkMessage_Response>(),
		m_DataTag(ChemlinkDataTag::Unknown),
		m_RawValue(0)
	{
	}

	ChemlinkMessage_Response::~ChemlinkMessage_Response() = default;

	ChemlinkDataTag ChemlinkMessage_Response::DataTag() const
	{
		return m_DataTag;
	}

	uint16_t ChemlinkMessage_Response::RawValue() const
	{
		return m_RawValue;
	}

	std::string ChemlinkMessage_Response::ToString() const
	{
		return std::format("Packet: {} || Payload: DataTag: {}, RawValue: {}", ChemlinkMessage::ToString(), magic_enum::enum_name(m_DataTag), m_RawValue);
	}

	bool ChemlinkMessage_Response::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool ChemlinkMessage_Response::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into ChemlinkMessage_Response type", message_bytes.size()));

		if (message_bytes.size() <= Index_DataTag)
		{
			LogDebug(Channel::Messages, "ChemlinkMessage_Response is too short to deserialise DataTag.");
			return false;
		}

		const auto raw_tag = message_bytes[Index_DataTag];
		auto tag = magic_enum::enum_cast<ChemlinkDataTag>(raw_tag);
		m_DataTag = tag.value_or(ChemlinkDataTag::Unknown);

		if (message_bytes.size() > Index_ValueHigh)
		{
			const uint8_t low = static_cast<uint8_t>(message_bytes[Index_ValueLow]);
			const uint8_t high = static_cast<uint8_t>(message_bytes[Index_ValueHigh]);
			m_RawValue = static_cast<uint16_t>(high * 256 + low);
		}
		else if (message_bytes.size() > Index_ValueLow)
		{
			m_RawValue = static_cast<uint16_t>(message_bytes[Index_ValueLow]);
		}

		return true;
	}

}
// namespace AqualinkAutomate::Messages
