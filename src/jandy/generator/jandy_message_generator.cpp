#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <string>

#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/factories/jandy_message_factory.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/generator/jandy_message_generator_buffercleanup.h"
#include "jandy/generator/jandy_message_generator_buffervalidation.h"
#include "jandy/generator/jandy_message_generator_packetprocessing.h"
#include "jandy/generator/jandy_message_generator_packetvalidation.h"
#include "jandy/types/jandy_types.h"
#include "jandy/utility/jandy_checksum.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Generators
{

	Types::JandyExpectedMessageType GenerateMessageFromRawData(std::vector<uint8_t>& serial_data)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Generating Message", BOOST_CURRENT_LOCATION);

		// Step 2 -> Read the bytes looking for a message header.

		if (!BufferValidation_ContainsMoreThanZeroBytes(serial_data))
		{
			LogTrace(Channel::Messages, "The internal serial data buffer is empty; ignoring message generation.");
			return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
		}
		else if (!BufferValidation_HasStartOfPacket(serial_data))
		{
			// Clear the internal buffer....it doesn't contain anything useful.
			serial_data.clear();

			LogTrace(Channel::Messages, "The internal serial data buffer does not contain a packet start sequence; ignoring message generation");
			return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
		}
		else
		{
			std::vector<uint8_t>::iterator packet_one_start_it, packet_one_end_it, packet_two_start_it;

			PacketProcessing_GetPacketLocations(serial_data, packet_one_start_it, packet_one_end_it, packet_two_start_it);
			BufferCleanUp_HasEndOfPacketWithinMaxDistance(serial_data, packet_one_start_it, packet_one_end_it, packet_two_start_it);

			// Buffer clean-up may have invalidated the iterators by erasing elements....
			PacketProcessing_GetPacketLocations(serial_data, packet_one_start_it, packet_one_end_it, packet_two_start_it);
			PacketProcessing_OutputSerialDataToConsole(serial_data, packet_one_start_it, packet_one_end_it, packet_two_start_it);

			if (serial_data.end() == packet_one_start_it)
			{
				// If no header found, clear all stored serial bytes and terminate
				serial_data.clear();

				LogTrace(Channel::Messages, "Cannot find start of packet in serial data; clearing serial buffer.");
				return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
			}
			else
			{
				const auto packet_one_start_found = (serial_data.end() != packet_one_start_it);
				const auto packet_one_end_found = (serial_data.end() != packet_one_end_it);
				const auto packet_two_start_found = (serial_data.end() != packet_two_start_it);

				if ((packet_one_end_found) && (packet_two_start_found) && (0 > std::distance(packet_one_end_it + 2, packet_two_start_it))) // Account for the DLE,ETX bytes
				{
					auto packet_one_end_index = std::distance(serial_data.begin(), packet_one_end_it);
					auto packet_two_start_index = std::distance(serial_data.begin(), packet_two_start_it);

					// There's an DLE,STX prior to this packets ETX,DLE...this is an error state.

					LogTrace(Channel::Messages, std::format("Searching for overlapping packets: packet one end: {}, packet two start: {}", packet_one_end_index, packet_two_start_index));
					LogTrace(Channel::Messages, "Found the start of a second packet before the current packet terminator bytes.");

					// Clear all stored serial bytes up to the start of the new packet.
					serial_data.erase(serial_data.begin(), serial_data.begin() + packet_two_start_index);
					return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
				}
				else if ((!packet_one_end_found) && (packet_two_start_found))
				{
					auto packet_two_start_index = std::distance(serial_data.begin(), packet_two_start_it);

					LogTrace(Channel::Messages, "Found the start of a second packet before the current packet terminator bytes.");

					// Clear all stored serial bytes up to the start of the new packet.
					serial_data.erase(serial_data.begin(), serial_data.begin() + packet_two_start_index);
					return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
				}
				else if (!packet_one_end_found)
				{
					// The packet is not complete....do nothing at this point.
					LogTrace(Channel::Messages, "End of current packet not yet received; awaiting more serial data.");
					return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
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
						BufferCleanUp_ClearBytesFromBeginToPos(serial_data, packet_one_end_it + 2);  // Account for the DLE,ETX bytes

						// Determine the return value.  This is going to be one of:
						//
						//    a) DataAvailableToProcess -> there is the start of the next packet in the buffer so processing should continue.
						//    b) WaitingForMoreData     -> more data needs to be read from the serial port before processing should continue.
						//
						
						if (packet_two_start_found)
						{
							return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
						}
						else
						{
							return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
						}
					}
					else
					{
						// Step 3b -> If checksum passes, convert to message, clear all bytes, go back to Step 2
						auto message_span = std::as_bytes(std::span(packet_one_start_it, packet_one_end_it + 2));
						auto message = Factory::JandyMessageFactory::Instance().CreateFromSerialData(message_span);

						BufferCleanUp_ClearBytesFromBeginToPos(serial_data, packet_one_end_it + 2);  // Account for the DLE,ETX bytes
						return message;
					}
				}
			}
		}

		// Getting here means there's been a problem processing the data...give up, erase the buffer, and ask for more data.
		serial_data.clear();

		LogDebug(Channel::Messages, "Unexpected failure while processing the internal serial data buffer; clearing serial data.");
		return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
	}

}
// namespace AqualinkAutomate::Generators
