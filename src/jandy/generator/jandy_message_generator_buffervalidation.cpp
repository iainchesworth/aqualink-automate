#include "jandy/generator/jandy_message_generator_buffervalidation.h"
#include "jandy/generator/jandy_message_generator_startendsequence.h"
#include "profiling/profiling.h"

namespace AqualinkAutomate::Generators
{

	bool BufferValidation_ContainsMoreThanZeroBytes(const std::vector<uint8_t>& serial_data)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Buffer Validation -> Has More Than Zero Bytes", std::source_location::current());

		return (0 != serial_data.size());
	}

	bool BufferValidation_HasStartOfPacket(const std::vector<uint8_t>& serial_data)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Buffer Validation -> Has Start Of Packet", std::source_location::current());

		return (serial_data.end() != std::search(serial_data.begin(), serial_data.end(), PACKET_START_SEQUENCE.begin(), PACKET_START_SEQUENCE.end()));
	}

}
// namespace AqualinkAutomate::Generators`
