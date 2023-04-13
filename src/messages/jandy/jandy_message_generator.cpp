#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <string>

#include "errors/message_error_codes.h"
#include "errors/protocol_error_codes.h"
#include "logging/logging.h"
#include "messages/jandy/jandy_message_factory.h"
#include "messages/jandy/jandy_message_generator.h"
#include "messages/jandy/jandy_message_processors.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;

namespace AqualinkAutomate::Messages::Jandy
{

	JandyMessageGenerator::JandyMessageGenerator() :
		MessageGenerator()
	{
	}

	boost::asio::awaitable<std::expected<JandyMessageGenerator::MessageType, boost::system::error_code>> JandyMessageGenerator::GenerateMessageFromRawData()
	{
		// Step 2 -> Read the bytes looking for a message header.

		if (!BufferValidation_ContainsMoreThanZeroBytes())
		{
			LogTrace(Channel::Messages, "The internal serial data buffer is empty; ignoring message generation.");
			co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
		}
		else if (!BufferValidation_HasStartOfPacket())
		{
			// Clear the internal buffer....it doesn't contain anything useful.
			m_SerialData.clear();

			LogTrace(Channel::Messages, "The internal serial data buffer does not contain a packet start sequence; ignoring message generation");
			co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
		}
		else
		{
			decltype(m_SerialData)::iterator packet_one_start_it, packet_one_end_it, packet_two_start_it;

			PacketProcessing_GetPacketLocations(packet_one_start_it, packet_one_end_it, packet_two_start_it);
			BufferCleanUp_HasEndOfPacketWithinMaxDistance(packet_one_start_it, packet_one_end_it, packet_two_start_it);

			// Buffer clean-up may have invalidated the iterators by erasing elements....
			PacketProcessing_GetPacketLocations(packet_one_start_it, packet_one_end_it, packet_two_start_it);
			PacketProcessing_OutputSerialDataToConsole(packet_one_start_it, packet_one_end_it, packet_two_start_it);

			if (m_SerialData.end() == packet_one_start_it)
			{
				// If no header found, clear all stored serial bytes and terminate
				m_SerialData.clear();

				LogTrace(Channel::Messages, "Cannot find start of packet in serial data; clearing serial buffer.");
				co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
			}
			else
			{
				const auto packet_one_start_found = (m_SerialData.end() != packet_one_start_it);
				const auto packet_one_end_found = (m_SerialData.end() != packet_one_end_it);
				const auto packet_two_start_found = (m_SerialData.end() != packet_two_start_it);

				if ((packet_one_end_found) && (packet_two_start_found) && (0 > std::distance(packet_one_end_it + 2, packet_two_start_it))) // Account for the DLE,ETX bytes
				{
					auto packet_one_end_index = std::distance(m_SerialData.begin(), packet_one_end_it);
					auto packet_two_start_index = std::distance(m_SerialData.begin(), packet_two_start_it);

					// There's an DLE,STX prior to this packets ETX,DLE...this is an error state.

					LogTrace(Channel::Messages, std::format("Searching for overlapping packets: packet one end: {}, packet two start: {}", packet_one_end_index, packet_two_start_index));
					LogTrace(Channel::Messages, "Found the start of a second packet before the current packet terminator bytes.");

					// Clear all stored serial bytes up to the start of the new packet.
					m_SerialData.erase(m_SerialData.begin(), m_SerialData.begin() + packet_two_start_index);
					co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
				}
				else if ((!packet_one_end_found) && (packet_two_start_found))
				{
					auto packet_two_start_index = std::distance(m_SerialData.begin(), packet_two_start_it);

					LogTrace(Channel::Messages, "Found the start of a second packet before the current packet terminator bytes.");

					// Clear all stored serial bytes up to the start of the new packet.
					m_SerialData.erase(m_SerialData.begin(), m_SerialData.begin() + packet_two_start_index);
					co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
				}
				else if (!packet_one_end_found)
				{
					// The packet is not complete....do nothing at this point.
					LogTrace(Channel::Messages, "End of current packet not yet received; awaiting more serial data.");
					co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
				}
				else
				{
					// Don't care, we may have found an DLE,STX but it is _after_ this packet's ETX,DLE
					auto packet_length = std::distance(packet_one_start_it, packet_one_end_it) + 2; // Account for the DLE,ETX bytes
					auto message_span = std::as_bytes(std::span(packet_one_start_it, packet_length));
					LogTrace(Channel::Messages, std::format("Aqualink packet (length: {} bytes) found in serial data.", message_span.size()));

					// Step 3 -> Validate that the packet is correctly formed and has not been corrupted
					if (!PacketValidation_ChecksumIsValid(message_span))
					{
						// Step 3a -> If checksum fails, clear bytes and terminate (which happens below)...
						LogDebug(Channel::Messages, "Packet failed checksum check; removing packet data from serial buffer.");
						BufferCleanUp_ClearBytesFromBeginToPos(packet_one_end_it + 2);  // Account for the DLE,ETX bytes

						// Determine the return value.  This is going to be one of:
						//
						//    a) DataAvailableToProcess -> there is the start of the next packet in the buffer so processing should continue.
						//    b) WaitingForMoreData     -> more data needs to be read from the serial port before processing should continue.
						//
						
						if (packet_two_start_found)
						{
							co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
						}
						else
						{
							co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
						}
					}
					else
					{
						// Step 3b -> If checksum passes, convert to message, clear all bytes, go back to Step 2
						auto message_span = std::as_bytes(std::span(packet_one_start_it, packet_one_end_it + 2));
						auto message = JandyMessageFactory::CreateFromSerialData(message_span);

						BufferCleanUp_ClearBytesFromBeginToPos(packet_one_end_it + 2);  // Account for the DLE,ETX bytes
						co_return message;
					}
				}
			}
		}

		// Getting here means there's been a problem processing the data...give up, erase the buffer, and ask for more data.
		m_SerialData.clear();

		LogDebug(Channel::Messages, "Unexpected failure while processing the internal serial data buffer; clearing serial data.");
		co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
	}

	bool JandyMessageGenerator::BufferValidation_ContainsMoreThanZeroBytes() const
	{
		return (0 != m_SerialData.size());
	}

	bool JandyMessageGenerator::BufferValidation_HasStartOfPacket() const
	{
		return (m_SerialData.end() != std::search(m_SerialData.begin(), m_SerialData.end(), m_PacketStartSeq.begin(), m_PacketStartSeq.end()));
	}

	void JandyMessageGenerator::BufferCleanUp_ClearBytesFromBeginToPos(const auto& position)
	{
		// Erase all bytes to the end of this packet.
		auto packet_one_end_index = std::distance(m_SerialData.begin(), position);
		LogTrace(Channel::Messages, "Packet processing complete; removing packet data from serial buffer.");
		LogTrace(Channel::Messages, std::format("Clearing {} elements from the serial data.", packet_one_end_index));
		m_SerialData.erase(m_SerialData.begin(), m_SerialData.begin() + packet_one_end_index);
	}

	void JandyMessageGenerator::BufferCleanUp_HasEndOfPacketWithinMaxDistance(const auto& p1s, const auto& p1e, const auto& p2s)
	{
		if (Messages::JandyMessage::MAXIMUM_PACKET_LENGTH >= m_SerialData.size())
		{
			// Not enough data in the buffer to do anything at this point in time...ignore.
		}
		else if (!BufferValidation_HasStartOfPacket())
		{
			// There doesn't appear to be a packet in this data...ignore.
		}
		else
		{
			// Do a bunch of sense checks...
			const auto distance_between_start_and_end = std::distance(p1s, p1e);
			if (0 == distance_between_start_and_end)
			{
				///TODO - this is an exceptional case!!!!!
				throw;
			}
			else if (0 > distance_between_start_and_end)
			{
				// The end is before the start...erase everything up to the start iterator.
				auto serial_data_begin = m_SerialData.cbegin();
				m_SerialData.erase(serial_data_begin, p1s);
			}
			else if (Messages::JandyMessage::MAXIMUM_PACKET_LENGTH < (distance_between_start_and_end + m_PacketEndSeq.size()))
			{
				LogDebug(Channel::Messages, std::format("Packet end sequence not present within {} bytes of start sequence; ignoring this particular packet", Messages::JandyMessage::MAXIMUM_PACKET_LENGTH));

				// The distance between the start and the end is larger than the maximum packet size...erase everything up to the end iterator (plus end bytes).
				auto serial_data_begin = m_SerialData.begin();
				m_SerialData.erase(serial_data_begin, p1e + m_PacketEndSeq.size());
			}
			else
			{
				// Buffer seems okay...continue and process it.
			}
		}
	}

	void JandyMessageGenerator::PacketProcessing_OutputSerialDataToConsole(auto& p1s, auto& p1e, auto& p2s)
	{
		std::string output_message;
		std::size_t elem_position = 0;

		auto packet_one_start_pos = std::distance(m_SerialData.begin(), p1s);
		auto packet_one_end_pos = std::distance(m_SerialData.begin(), p1e) + 1; // Account for the length of the footer bytes.
		auto packet_two_start_pos = std::distance(m_SerialData.begin(), p2s);

		for (const auto& elem : m_SerialData)
		{
			if ((m_SerialData.end() != p1s) && (packet_one_start_pos == elem_position))
			{
				output_message.append("->");
			}

			if ((m_SerialData.end() != p2s) && (packet_two_start_pos == elem_position))
			{
				output_message.append("|| =>");
			}

			output_message.append(std::format("{:02x}", elem));

			if ((m_SerialData.end() != p1e) && (packet_one_end_pos == elem_position))
			{
				output_message.append("<-");
			}

			output_message.append(" ");
			elem_position++;
		}

		LogTrace(Channel::Messages, std::format("Serial Data: {}", output_message));
	}

	std::vector<uint8_t>::iterator JandyMessageGenerator::PacketProcessing_GetPacketLocation(const auto& needle)
	{
		return std::search(m_SerialData.begin(), m_SerialData.end(), needle.begin(), needle.end());
	}

	void JandyMessageGenerator::PacketProcessing_GetPacketLocations(auto& p1s, auto& p1e, auto& p2s)
	{
		p1e = m_SerialData.end();
		p2s = m_SerialData.end();

		if (p1s = std::search(m_SerialData.begin(), m_SerialData.end(), m_PacketStartSeq.begin(), m_PacketStartSeq.end()); m_SerialData.end() == p1s)
		{
			// Given there's no start of a packet...ignore any searching for the end or a second packet start.
		}
		else
		{
			p1e = std::search(p1s + 1, m_SerialData.end(), m_PacketEndSeq.begin(), m_PacketEndSeq.end());
			p2s = std::search(p1s + 1, m_SerialData.end(), m_PacketStartSeq.begin(), m_PacketStartSeq.end());
		}
	}

	bool JandyMessageGenerator::PacketValidation_ChecksumIsValid(const auto& message_span) const
	{
		const auto length_minus_checksum_and_footer = message_span.size() - 3;
		const uint8_t original_checksum = static_cast<uint8_t>(message_span[length_minus_checksum_and_footer]);
		const auto span_to_check = message_span.first(length_minus_checksum_and_footer);

		uint32_t checksum = 0;

		for (auto& elem : span_to_check)
		{
			checksum += static_cast<uint32_t>(elem);
		}

		const auto calculated_checksum = static_cast<uint8_t>(checksum & 0xFF);
		const auto checksum_is_valid = (original_checksum == calculated_checksum);

		LogTrace(Channel::Messages, std::format("Validating packet checksum: calculated=0x{:02x}, original=0x{:02x} -> {}!", calculated_checksum, original_checksum, checksum_is_valid ? "Success" : "Failure"));

		return checksum_is_valid;
	}

}
// namespace AqualinkAutomate::Messages::Jandy
