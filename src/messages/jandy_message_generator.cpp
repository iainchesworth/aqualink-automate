#include <algorithm>
#include <cstddef>
#include <span>

#include "messages/jandy_message_generator.h"

namespace AqualinkAutomate::Messages
{

	JandyMessageGenerator::JandyMessageGenerator() : 
		MessageGenerator(), 
		m_MSM()
	{
		m_MSM.initiate();
	}

	std::expected<MessageGenerator<>::Message, uint32_t> JandyMessageGenerator::GenerateMessageFromRawData(const std::array<uint8_t, 16>& read_buffer, std::size_t bytes_read)
	{
		// Step 1 -> push the new bytes into the internal buffer.
		m_RawMessageBuffer.insert(m_RawMessageBuffer.end(), std::begin(read_buffer), std::begin(read_buffer)+bytes_read);
		
		// Step 2 -> Read the bytes looking for a message header.
		auto seq_one_span = std::as_bytes(std::span(m_RawMessageBuffer));
		auto seq_one = std::array<std::byte, 2>{static_cast<std::byte>(JandyMessageHeaderBytes::DLE), static_cast<std::byte>(JandyMessageHeaderBytes::STX)};
		auto seq_one_begin = std::begin(seq_one);
		auto seq_one_end = std::end(seq_one);
				
		if (auto dle_stx_it = std::search(seq_one_span.begin(), seq_one_span.end(), seq_one_begin, seq_one_end); seq_one_span.end() == dle_stx_it)
		{
			// Step 2a -> If no header found, clear bytes and terminate
			m_RawMessageBuffer.clear();
			m_MSM.process_event(InvalidPacket());
		}
		else
		{	
			m_MSM.process_event(STX_Received());

			// Step 2b -> If header found, clear all preceding bytes
			auto index = std::distance(seq_one_span.begin(), dle_stx_it);
			m_RawMessageBuffer.erase(m_RawMessageBuffer.begin(), m_RawMessageBuffer.begin()+index);

			// Step 3 -> Read until end of message (based on length in header)
			auto seq_two_span = std::as_bytes(std::span(m_RawMessageBuffer));
			auto seq_two = std::array<std::byte, 2>{static_cast<std::byte>(JandyMessageHeaderBytes::ETX), static_cast<std::byte>(JandyMessageHeaderBytes::DLE)};
			auto seq_two_begin = std::begin(seq_two);
			auto seq_two_end = std::end(seq_two);

			if (auto etx_dle_it = std::search(seq_two_span.begin(), seq_two_span.end(), seq_two_begin, seq_two_end); seq_two_span.end() == etx_dle_it)
			{
				m_MSM.process_event(ETX_Received());
				m_MSM.process_event(DLE_Received());

				// Step 3a -> If checksum fails, clear bytes and terminate
				m_MSM.process_event(InvalidPacket());

				// Step 3b -> If checksum passes, convert to message, clear all bytes, go back to Step 2
				///TODO
			}
			else
			{
				auto index = std::distance(seq_two_span.begin(), etx_dle_it);
			}
		}

		return std::unexpected<uint32_t>(0);
	}

}
// namespace AqualinkAutomate::Messages
