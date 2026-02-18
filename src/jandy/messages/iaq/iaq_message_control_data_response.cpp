#include <format>

#include "messages/iaq/iaq_message_control_data_response.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_ControlDataResponse::IAQMessage_ControlDataResponse(const std::string& ascii_data) :
		IAQMessage(JandyMessageIds::IAQ_PageButton),
		Interfaces::IMessageSignalSend<IAQMessage_ControlDataResponse>(),
		m_AsciiData(ascii_data)
	{
		m_Destination = Devices::JandyDeviceType(0x00);
	}

	IAQMessage_ControlDataResponse::~IAQMessage_ControlDataResponse() = default;

	std::string IAQMessage_ControlDataResponse::ToString() const
	{
		return std::format("Packet: {} || Payload: ControlDataResponse('{}')", IAQMessage::ToString(), m_AsciiData);
	}

	bool IAQMessage_ControlDataResponse::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		size_t bytes_written = 0;

		for (size_t i = 0; i < DATA_PAYLOAD_SIZE; ++i)
		{
			if (i < m_AsciiData.size())
			{
				message_bytes.emplace_back(static_cast<uint8_t>(m_AsciiData[i]));
			}
			else
			{
				message_bytes.emplace_back(0x00);
			}
			++bytes_written;
		}

		LogDebug(Channel::Messages, std::format("Serialised IAQMessage_ControlDataResponse: {} bytes, data='{}'", bytes_written, m_AsciiData));

		return true;
	}

	bool IAQMessage_ControlDataResponse::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		// Send-only message; deserialization is not supported.
		return false;
	}

}
// namespace AqualinkAutomate::Messages
