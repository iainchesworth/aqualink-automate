#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace AqualinkAutomate::Utility
{

	void JandyPacket_NullCharHandler_Serialization(std::vector<uint8_t>& message_bytes);
	void JandyPacket_NullCharHandler_Deserialization(std::vector<uint8_t>& message_bytes);

	/// Returns true if the packet contains any DLE-null escape sequences
	/// (0x10 followed by 0x00) that require removal during deserialization.
	bool JandyPacket_NeedsNullCharHandling(std::span<const uint8_t> message_bytes);

	/// Copies message_bytes into output while removing DLE-null escape
	/// sequences.  Returns the number of bytes written to output.
	std::size_t JandyPacket_NullCharHandler_DeserializationToSpan(std::span<const uint8_t> input, std::span<uint8_t> output);

}
// namespace AqualinkAutomate::Utility
