#include <format>

#include "generator/jandy_message_generator_packetprocessing.h"
#include "generator/jandy_message_generator_startendsequence.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Generators
{

	PacketLocations PacketProcessing_FindAllPacketLocations(boost::circular_buffer<uint8_t>& serial_data)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator::PacketProcessing -> find_all_locations", std::source_location::current());

		PacketLocations loc;
		loc.p1_start = serial_data.end();
		loc.p1_end = serial_data.end();
		loc.p2_start = serial_data.end();
		loc.HasPacketStart = false;
		loc.HasPacketEnd = false;
		loc.HasSecondPacketStart = false;

		// Single pass: find p1_start, p1_end, and p2_start
		loc.p1_start = std::search(serial_data.begin(), serial_data.end(), PACKET_START_SEQUENCE.begin(), PACKET_START_SEQUENCE.end());

		if (serial_data.end() == loc.p1_start)
		{
			return loc;
		}

		loc.HasPacketStart = true;

		// Search for end sequence and second start sequence from p1_start + 1
		auto search_from = loc.p1_start + 1;
		loc.p1_end = std::search(search_from, serial_data.end(), PACKET_END_SEQUENCE.begin(), PACKET_END_SEQUENCE.end());
		loc.p2_start = std::search(search_from, serial_data.end(), PACKET_START_SEQUENCE.begin(), PACKET_START_SEQUENCE.end());

		loc.HasPacketEnd = (serial_data.end() != loc.p1_end);
		loc.HasSecondPacketStart = (serial_data.end() != loc.p2_start);

		return loc;
	}

	void PacketProcessing_OutputSerialDataToConsole(const boost::circular_buffer<uint8_t>& serial_data, const PacketLocations& locations)
	{
		auto zone1 = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator::PacketProcessing -> output_to_console", std::source_location::current());

		thread_local std::string output_message = []()
			{
				std::string s;
				s.reserve(512);
				return s;
			}();

		output_message.clear();

		{
			auto zone2 = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator::PacketProcessing -> output_to_console -> generate", std::source_location::current());

			std::size_t elem_position = 0;

			auto packet_one_start_pos = std::distance<boost::circular_buffer<uint8_t>::const_iterator>(serial_data.cbegin(), locations.p1_start);
			auto packet_one_end_pos = std::distance<boost::circular_buffer<uint8_t>::const_iterator>(serial_data.cbegin(), locations.p1_end) + 1; // Account for the length of the footer bytes.
			auto packet_two_start_pos = std::distance<boost::circular_buffer<uint8_t>::const_iterator>(serial_data.cbegin(), locations.p2_start);

			for (const auto& elem : serial_data)
			{
				if ((locations.HasPacketStart) && (packet_one_start_pos == elem_position))
				{
					output_message.append("->");
				}

				if ((locations.HasSecondPacketStart) && (packet_two_start_pos == elem_position))
				{
					output_message.append("|| =>");
				}

				output_message.append(std::format("{:02x}", elem));

				if ((locations.HasPacketEnd) && (packet_one_end_pos == elem_position))
				{
					output_message.append("<-");
				}

				output_message.append(" ");
				elem_position++;
			}

			{
				auto zone3 = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator::PacketProcessing -> output_to_console -> logging", std::source_location::current());
				LogTrace(Channel::Messages, std::format("Serial Data: {}", output_message));
			}
		}
	}

}
// namespace AqualinkAutomate::Generators
