#pragma once

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace AqualinkAutomate::Pentair::Utility
{

	template <typename T>
	concept ByteLikeIntegral = std::integral<std::remove_cv_t<T>> || std::same_as<std::remove_cv_t<T>, std::byte>;

	// Compute the Pentair 16-bit frame checksum: the running sum of every byte in
	// the range (the checksummed region runs from the 0xA5 SOF through the final
	// DATA byte).  The wire encoding is big-endian (high byte first, low byte
	// second), so callers append (checksum >> 8) then (checksum & 0xFF).
	template <std::ranges::input_range Range>
		requires ByteLikeIntegral<std::ranges::range_value_t<Range>>
	uint16_t PentairPacket_CalculateChecksum_FromRange(Range&& message_bytes)
	{
		uint32_t checksum = 0;

		for (auto&& elem : message_bytes)
		{
			using value_type = std::remove_cvref_t<decltype(elem)>;

			uint32_t value = 0;
			if constexpr (std::same_as<value_type, std::byte>)
			{
				value = static_cast<uint32_t>(std::to_integer<uint8_t>(elem));
			}
			else
			{
				value = static_cast<uint32_t>(static_cast<uint8_t>(elem));
			}

			checksum += value;
		}

		return static_cast<uint16_t>(checksum & 0xFFFF);
	}

	template <std::input_iterator It>
		requires ByteLikeIntegral<std::iter_value_t<It>>
	uint16_t PentairPacket_CalculateChecksum(It first, It last)
	{
		auto subrange = std::ranges::subrange(first, last);
		return PentairPacket_CalculateChecksum_FromRange(subrange);
	}

}
// namespace AqualinkAutomate::Pentair::Utility
