#pragma once

#include <cstddef>

namespace AqualinkAutomate::Protocol
{

	namespace Constants
	{

		constexpr std::size_t SERIAL_CIRCULAR_BUFFER_SIZE = 4096;
		constexpr std::size_t SERIAL_READ_CHUNK_SIZE = 128;

	}
	// namespace Constants

}
// namespace AqualinkAutomate::Protocol
