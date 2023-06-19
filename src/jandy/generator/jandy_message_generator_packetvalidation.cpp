#include <cstdint>

#include "jandy/generator/jandy_message_generator_packetvalidation.h"
#include "jandy/utility/jandy_checksum.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Generators
{

	bool PacketValidation_ChecksumIsValid(const std::span<const std::byte>& message_span)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JandyMessageGenerator -> Packet Validation -> Validate Checksum", std::source_location::current());

		const auto length_minus_checksum_and_footer = message_span.size() - 3;
		const uint8_t original_checksum = static_cast<uint8_t>(message_span[length_minus_checksum_and_footer]);
		const auto span_to_check = message_span.first(length_minus_checksum_and_footer);
		const auto calculated_checksum = Utility::JandyPacket_CalculateChecksum(span_to_check);
		const auto checksum_is_valid = (original_checksum == calculated_checksum);

		LogTrace(Channel::Messages, std::format("Validating packet checksum: calculated=0x{:02x}, original=0x{:02x} -> {}!", calculated_checksum, original_checksum, checksum_is_valid ? "Success" : "Failure"));

		return checksum_is_valid;
	}

}
// namespace AqualinkAutomate::Generators`
