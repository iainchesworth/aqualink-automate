#include <format>

#include "jandy/generator/jandy_message_generator_packetprocessing.h"
#include "jandy/generator/jandy_message_generator_startendsequence.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Generators
{
	
	void PacketProcessing_GetPacketLocations(std::vector<uint8_t>& serial_data, std::vector<uint8_t>::iterator& p1s, std::vector<uint8_t>::iterator& p1e, std::vector<uint8_t>::iterator& p2s)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Packet Processing -> Get Packet Location (Multiple)", std::source_location::current());

		p1e = serial_data.end();
		p2s = serial_data.end();

		if (p1s = std::search(serial_data.begin(), serial_data.end(), PACKET_START_SEQUENCE.begin(), PACKET_START_SEQUENCE.end()); serial_data.end() == p1s)
		{
			// Given there's no start of a packet...ignore any searching for the end or a second packet start.
		}
		else
		{
			p1e = std::search(p1s + 1, serial_data.end(), PACKET_END_SEQUENCE.begin(), PACKET_END_SEQUENCE.end());
			p2s = std::search(p1s + 1, serial_data.end(), PACKET_START_SEQUENCE.begin(), PACKET_START_SEQUENCE.end());
		}
	}

	void PacketProcessing_OutputSerialDataToConsole(const std::vector<uint8_t>& serial_data, const std::vector<uint8_t>::iterator& p1s, const std::vector<uint8_t>::iterator& p1e, const std::vector<uint8_t>::iterator& p2s)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Packet Processing -> Output To Console", std::source_location::current());

		std::string output_message;
		std::size_t elem_position = 0;

		auto packet_one_start_pos = std::distance<std::vector<uint8_t>::const_iterator>(serial_data.cbegin(), p1s);
		auto packet_one_end_pos = std::distance<std::vector<uint8_t>::const_iterator>(serial_data.cbegin(), p1e) + 1; // Account for the length of the footer bytes.
		auto packet_two_start_pos = std::distance<std::vector<uint8_t>::const_iterator>(serial_data.cbegin(), p2s);

		for (const auto& elem : serial_data)
		{
			if ((serial_data.end() != p1s) && (packet_one_start_pos == elem_position))
			{
				output_message.append("->");
			}

			if ((serial_data.end() != p2s) && (packet_two_start_pos == elem_position))
			{
				output_message.append("|| =>");
			}

			output_message.append(std::format("{:02x}", elem));

			if ((serial_data.end() != p1e) && (packet_one_end_pos == elem_position))
			{
				output_message.append("<-");
			}

			output_message.append(" ");
			elem_position++;
		}

		LogTrace(Channel::Messages, std::format("Serial Data: {}", output_message));
	}

}
// namespace AqualinkAutomate::Generators`
