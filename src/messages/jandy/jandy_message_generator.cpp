#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <string>

#include "errors/message_error_codes.h"
#include "errors/protocol_error_codes.h"
#include "logging/logging.h"
#include "messages/jandy/jandy_message_constants.h"
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

	boost::asio::awaitable<std::expected<JandyMessageGenerator::MessageType, JandyMessageGenerator::ErrorType>> JandyMessageGenerator::GenerateMessageFromRawData()
	{
		// Step 2 -> Read the bytes looking for a message header.
		auto serial_data_span = std::as_bytes(std::span(m_SerialData));

		const auto packet_start_sequence = std::array<std::byte, 2>{static_cast<std::byte>(HEADER_BYTE_DLE), static_cast<std::byte>(HEADER_BYTE_STX)};
		const auto packet_end_sequence = std::array<std::byte, 2>{static_cast<std::byte>(HEADER_BYTE_DLE), static_cast<std::byte>(HEADER_BYTE_ETX)};

		auto packet_one_start_it = std::search(serial_data_span.begin(), serial_data_span.end(), packet_start_sequence.begin(), packet_start_sequence.end());
		auto packet_one_end_it = std::search(packet_one_start_it + 1, serial_data_span.end(), packet_end_sequence.begin(), packet_end_sequence.end());
		auto packet_two_start_it = std::search(packet_one_start_it + 1, serial_data_span.end(), packet_start_sequence.begin(), packet_start_sequence.end());

		auto OutputSerialDataIntoTraceLogging = [&]()
		{
			std::string output_message;
			std::size_t elem_position = 0;

			auto packet_one_start_pos = std::distance(serial_data_span.begin(), packet_one_start_it);
			auto packet_one_end_pos = std::distance(serial_data_span.begin(), packet_one_end_it) + 1; // Account for the length of the footer bytes.
			auto packet_two_start_pos = std::distance(serial_data_span.begin(), packet_two_start_it);

			for (const auto& elem : m_SerialData)
			{
				if ((serial_data_span.end() != packet_one_start_it) && (packet_one_start_pos == elem_position))
				{
					output_message.append("->");
				}

				if ((serial_data_span.end() != packet_two_start_it) && (packet_two_start_pos == elem_position))
				{
					output_message.append("|| =>");
				}

				output_message.append(std::format("{:02x}", elem));

				if ((serial_data_span.end() != packet_one_end_it) && (packet_one_end_pos == elem_position))
				{
					output_message.append("<-");
				}

				output_message.append(" ");
				elem_position++;
			}

			LogTrace(Channel::Messages, std::format("Serial Data: {}", output_message));
		};

		OutputSerialDataIntoTraceLogging();

		if (serial_data_span.end() == packet_one_start_it)
		{
			LogDebug(Channel::Messages, "Cannot find start of Aqualink packet; clearing serial buffer.");

			// If no header found, clear all stored serial bytes and terminate
			m_SerialData.clear();
		}
		else
		{
			const auto packet_one_start_found = (serial_data_span.end() != packet_one_start_it);
			const auto packet_one_end_found = (serial_data_span.end() != packet_one_end_it);
			const auto packet_two_start_found = (serial_data_span.end() != packet_two_start_it);

			if ((packet_one_end_found) && (packet_two_start_found) && (0 > std::distance(packet_one_end_it + 2, packet_two_start_it))) // Account for the DLE,ETX bytes
			{
				auto packet_one_end_index = std::distance(serial_data_span.begin(), packet_one_end_it);
				auto packet_two_start_index = std::distance(serial_data_span.begin(), packet_two_start_it);

				// There's an DLE,STX prior to this packets ETX,DLE...this is an error state.

				LogTrace(Channel::Messages, std::format("Searching for overlapping packets: packet one end: {}, packet two start: {}", packet_one_end_index, packet_two_start_index));
				LogDebug(Channel::Messages, "Found the start of a second packet before the current packet terminator bytes.");

				// Clear all stored serial bytes up to the start of the new packet.
				m_SerialData.erase(m_SerialData.begin(), m_SerialData.begin() + packet_two_start_index);
			}
			else if ((!packet_one_end_found) && (packet_two_start_found))
			{
				auto packet_two_start_index = std::distance(serial_data_span.begin(), packet_two_start_it);

				LogDebug(Channel::Messages, "Found the start of a second packet before the current packet terminator bytes.");

				// Clear all stored serial bytes up to the start of the new packet.
				m_SerialData.erase(m_SerialData.begin(), m_SerialData.begin() + packet_two_start_index);
			}
			else if (!packet_one_end_found)
			{
				// The packet is not complete....do nothing at this point.
				LogDebug(Channel::Messages, "End of current packet not yet received; awaiting more serial data.");
			}
			else
			{
				// Don't care, we may have found an DLE,STX but it is _after_ this packet's ETX,DLE
				auto packet_length = std::distance(packet_one_start_it, packet_one_end_it) + 2; // Account for the DLE,ETX bytes
				auto message_span = std::as_bytes(std::span(packet_one_start_it, packet_length));
				LogTrace(Channel::Messages, std::format("Aqualink packet (length: {} bytes) found in serial data.", message_span.size()));

				// Step 3 -> Validate that the packet is correctly formed and has not been corrupted
				auto validate_checksum = [](const auto& message_span) -> bool
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
				};

				auto clear_bytes_from_serial_buffer = [&]() -> void
				{
					// Erase all bytes to the end of this packet.
					auto packet_one_end_index = std::distance(serial_data_span.begin(), packet_one_end_it + 2);  // Account for the DLE,ETX bytes
					LogDebug(Channel::Messages, "Packet processing complete; removing packet data from serial buffer.");
					LogTrace(Channel::Messages, std::format("Clearing {} elements from the serial data.", packet_one_end_index));
					m_SerialData.erase(m_SerialData.begin(), m_SerialData.begin() + packet_one_end_index);
				};

				if (!validate_checksum(message_span))
				{
					// Step 3a -> If checksum fails, clear bytes and terminate (which happens below)...
					LogDebug(Channel::Messages, "Packet failed checksum check; removing packet data from serial buffer.");
					clear_bytes_from_serial_buffer();
				}
				else
				{
					// Step 3b -> If checksum passes, convert to message, clear all bytes, go back to Step 2
					auto message_span = std::span(packet_one_start_it + 2, packet_one_end_it - 1);
					auto message = JandyMessageFactory::CreateFromSerialData(message_span);
					
					clear_bytes_from_serial_buffer();
					co_return message;
				}
			}
		}

		// Determine the return value.  This is going to be one of:
		//
		//    a) DataAvailableToProcess -> there is the start of the next packet in the buffer so processing should continue.
		//    b) WaitingForMoreData     -> more data needs to be read from the serial port before processing should continue.
		//

		if (serial_data_span.end() != packet_two_start_it)
		{
			co_return std::unexpected<JandyMessageGenerator::ErrorType>(ErrorCodes::Protocol::DataAvailableToProcess());
		}
		else
		{
			co_return std::unexpected<JandyMessageGenerator::ErrorType>(ErrorCodes::Protocol::WaitingForMoreData());
		}
	}

}
// namespace AqualinkAutomate::Messages::Jandy
