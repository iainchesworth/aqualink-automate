#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <ranges>
#include <vector>

#include "errors/message_errors.h"
#include "errors/protocol_errors.h"
#include "factories/pentair_message_factory.h"
#include "generator/pentair_message_generator.h"
#include "messages/pentair_message_constants.h"
#include "utility/pentair_checksum.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Pentair::Generators
{

	namespace
	{
		// The 0xFF 0x00 0xFF 0xA5 sequence that opens every Pentair frame.
		constexpr std::array<uint8_t, 4> PREAMBLE = Messages::FRAME_PREAMBLE;
	}

	Types::PentairExpectedMessageType GenerateMessageFromRawData(boost::circular_buffer<uint8_t>& serial_data)
	{
		using ErrorCodes::Protocol_ErrorCodes;

		if (serial_data.empty())
		{
			return std::unexpected(make_error_code(Protocol_ErrorCodes::WaitingForMoreData));
		}

		// Locate the Pentair preamble.  If it is not present, leave the buffer
		// untouched so a co-registered generator (Jandy) can claim these bytes.
		auto preamble_it = std::search(serial_data.begin(), serial_data.end(), PREAMBLE.begin(), PREAMBLE.end());
		if (serial_data.end() == preamble_it)
		{
			LogTrace(Channel::Messages, "No Pentair preamble found in serial buffer; deferring to other protocols");
			return std::unexpected(make_error_code(Protocol_ErrorCodes::WaitingForMoreData));
		}

		// The 0xA5 SOF is the last byte of the located preamble; the checksummed
		// region begins there.
		const auto sof_it = preamble_it + (PREAMBLE.size() - 1);
		const auto bytes_from_sof = static_cast<std::size_t>(std::distance(sof_it, serial_data.end()));

		// Need at least the fixed header to read the LEN field.
		if (Messages::HEADER_LENGTH > bytes_from_sof)
		{
			LogTrace(Channel::Messages, "Pentair preamble found but header incomplete; awaiting more serial data");
			return std::unexpected(make_error_code(Protocol_ErrorCodes::WaitingForMoreData));
		}

		const uint8_t data_length = *(sof_it + Messages::Offset_Length);
		const std::size_t frame_region_size =
			static_cast<std::size_t>(Messages::HEADER_LENGTH) + data_length + Messages::CHECKSUM_LENGTH;

		// Whole checksummed region (0xA5 .. CHK_LO) must be present.
		if (frame_region_size > bytes_from_sof)
		{
			LogTrace(Channel::Messages, std::format("Pentair frame incomplete: need {} bytes from SOF, have {}; awaiting more data", frame_region_size, bytes_from_sof));
			return std::unexpected(make_error_code(Protocol_ErrorCodes::WaitingForMoreData));
		}

		// Copy the checksummed region into a contiguous buffer (the circular
		// buffer may wrap).
		std::vector<uint8_t> frame_region;
		frame_region.reserve(frame_region_size);
		std::copy_n(sof_it, frame_region_size, std::back_inserter(frame_region));

		// How many bytes (preamble + region) we will consume from the buffer
		// front on a definitive outcome (success or checksum failure).
		const auto consume_count = static_cast<std::size_t>(std::distance(serial_data.begin(), sof_it)) + frame_region_size;

		// Validate the checksum before constructing a message.
		const std::span<const uint8_t> region_to_checksum(frame_region.data(), frame_region.size() - Messages::CHECKSUM_LENGTH);
		const uint16_t expected_checksum = Utility::PentairPacket_CalculateChecksum_FromRange(region_to_checksum);
		const uint16_t received_checksum = static_cast<uint16_t>(
			(static_cast<uint16_t>(frame_region[frame_region.size() - Messages::CHECKSUM_LENGTH]) << 8) |
			static_cast<uint16_t>(frame_region[frame_region.size() - 1]));

		if (expected_checksum != received_checksum)
		{
			LogDebug(Channel::Messages, std::format("Pentair frame failed checksum check (expected 0x{:04x}, received 0x{:04x}); discarding frame", expected_checksum, received_checksum));
			serial_data.erase(serial_data.begin(), serial_data.begin() + static_cast<std::ptrdiff_t>(consume_count));
			return std::unexpected(make_error_code(Protocol_ErrorCodes::ChecksumFailure));
		}

		// Build and deserialise the message from the validated region.
		auto message = Factory::PentairMessageFactory::CreateFromSerialData(frame_region);

		// Consume the frame (and any leading bytes ahead of the preamble) now
		// that we have a definitive result.
		serial_data.erase(serial_data.begin(), serial_data.begin() + static_cast<std::ptrdiff_t>(consume_count));

		return message;
	}

}
// namespace AqualinkAutomate::Pentair::Generators
