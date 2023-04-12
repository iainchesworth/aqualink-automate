#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <span>

#include "logging/logging.h"
#include "messages/jandy/jandy_message_constants.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy
{

	template<typename SPAN_OF_BYTES>
	auto FindStartOfPacket(const SPAN_OF_BYTES& span_of_bytes)
	{
		auto seq_one = std::array<std::byte, 2>{static_cast<std::byte>(HEADER_BYTE_DLE), static_cast<std::byte>(HEADER_BYTE_STX)};
		auto seq_one_begin = std::begin(seq_one);
		auto seq_one_end = std::end(seq_one);

		return std::search(span_of_bytes.begin(), span_of_bytes.end(), seq_one_begin, seq_one_end);
	}

	template<typename SPAN_OF_BYTES>
	auto FindEndOfPacket(const SPAN_OF_BYTES& span_of_bytes)
	{
		auto seq_two = std::array<std::byte, 2>{static_cast<std::byte>(HEADER_BYTE_ETX), static_cast<std::byte>(HEADER_BYTE_DLE)};
		auto seq_two_begin = std::begin(seq_two);
		auto seq_two_end = std::end(seq_two);

		return std::search(span_of_bytes.begin(), span_of_bytes.end(), seq_two_begin, seq_two_end);
	}

	template<typename SPAN_OF_BYTES>
	auto ValidatePacketChecksum(const SPAN_OF_BYTES& message)
	{
		const uint8_t original_checksum = static_cast<uint8_t>(message[message.size() - 3]);
		const auto span_to_check = message.first(message.size() - 3);
	
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

}
// namespace AqualinkAutomate::Messages::Jandy
