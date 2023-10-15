#include <algorithm>
#include <format>

#include "jandy/generator/jandy_message_generator_buffercleanup.h"
#include "jandy/generator/jandy_message_generator_buffervalidation.h"
#include "jandy/generator/jandy_message_generator_startendsequence.h"
#include "jandy/messages/jandy_message.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Generators
{

	void BufferCleanUp_ClearBytesFromBeginToPos(std::vector<uint8_t>& serial_data, const std::vector<uint8_t>::iterator& position)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Buffer CleanUp -> Clearing Bytes", BOOST_CURRENT_LOCATION);

		// Erase all bytes to the end of this packet.
		auto packet_one_end_index = std::distance(serial_data.begin(), position);
		LogTrace(Channel::Messages, "Packet processing complete; removing packet data from serial buffer.");
		LogTrace(Channel::Messages, std::format("Clearing {} elements from the serial data.", packet_one_end_index));
		serial_data.erase(serial_data.begin(), serial_data.begin() + packet_one_end_index);
	}

	void BufferCleanUp_HasEndOfPacketWithinMaxDistance(std::vector<uint8_t>& serial_data, const std::vector<uint8_t>::iterator& p1s, const std::vector<uint8_t>::iterator& p1e, const std::vector<uint8_t>::iterator& p2s)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Buffer CleanUp -> Has End Of Packet", BOOST_CURRENT_LOCATION);

		if (Messages::JandyMessage::MAXIMUM_PACKET_LENGTH >= serial_data.size())
		{
			// Not enough data in the buffer to do anything at this point in time...ignore.
		}
		else if (!BufferValidation_HasStartOfPacket(serial_data))
		{
			// There doesn't appear to be a packet in this data...ignore.
		}
		else
		{
			// Do a bunch of sense checks...
			const auto distance_between_start_and_end = std::distance(p1s, p1e);
			if (0 == distance_between_start_and_end)
			{
				LogDebug(Channel::Messages, "Attempted to action a clean-up here the start and end location were the same...ignoring");
			}
			else if (0 > distance_between_start_and_end)
			{
				// The end is before the start...erase everything up to the start iterator.
				auto serial_data_begin = serial_data.cbegin();
				serial_data.erase(serial_data_begin, p1s);
			}
			else if (Messages::JandyMessage::MAXIMUM_PACKET_LENGTH < (distance_between_start_and_end + PACKET_END_SEQUENCE.size()))
			{
				LogDebug(Channel::Messages, std::format("Packet end sequence not present within {} bytes of start sequence; ignoring this particular packet", Messages::JandyMessage::MAXIMUM_PACKET_LENGTH));

				// The distance between the start and the end is larger than the maximum packet size...erase everything up to the end iterator (plus end bytes).
				auto serial_data_begin = serial_data.begin();
				serial_data.erase(serial_data_begin, p1e + PACKET_END_SEQUENCE.size());
			}
			else
			{
				// Buffer seems okay...continue and process it.
			}
		}
	}

}
// namespace AqualinkAutomate::Generators`
