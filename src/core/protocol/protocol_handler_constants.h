#pragma once

#include <chrono>
#include <cstddef>

namespace AqualinkAutomate::Protocol
{

	namespace Constants
	{

		constexpr std::size_t SERIAL_CIRCULAR_BUFFER_SIZE = 4096;
		constexpr std::size_t SERIAL_READ_CHUNK_SIZE = 16;
		constexpr std::chrono::milliseconds SERIAL_READ_TIMEOUT_DURATION{ std::chrono::milliseconds(100) };
		constexpr std::chrono::seconds SERIAL_WRITE_TIMEOUT_DURATION{ std::chrono::seconds(2) };

	}
	// namespace Constants

}
// namespace AqualinkAutomate::Protocol
