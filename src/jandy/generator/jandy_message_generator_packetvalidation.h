#pragma once

#include <cstdint>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Generators
{
	bool PacketValidation_ChecksumIsValid(boost::circular_buffer<uint8_t>::iterator start_it, boost::circular_buffer<uint8_t>::iterator end_it);

}
// namespace AqualinkAutomate::Generators
