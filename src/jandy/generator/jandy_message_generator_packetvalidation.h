#pragma once

#include <cstddef>
#include <span>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Generators
{
	bool PacketValidation_ChecksumIsValid(boost::circular_buffer<uint8_t>::iterator start_it, boost::circular_buffer<uint8_t>::iterator end_it);
	//FIXME bool PacketValidation_ChecksumIsValid(const std::span<const std::byte>& message_span);

}
// namespace AqualinkAutomate::Generators
