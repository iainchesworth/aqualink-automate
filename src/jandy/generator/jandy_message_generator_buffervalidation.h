#pragma once

#include <cstdint>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Generators
{

	bool BufferValidation_ContainsMoreThanZeroBytes(const boost::circular_buffer<uint8_t>& serial_data);
	bool BufferValidation_HasStartOfPacket(const boost::circular_buffer<uint8_t>& serial_data);

}
// namespace AqualinkAutomate::Generators
