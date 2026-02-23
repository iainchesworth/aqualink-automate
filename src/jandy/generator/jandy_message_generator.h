#pragma once

#include <cstdint>

#include <boost/circular_buffer.hpp>

#include "types/jandy_types.h"

namespace AqualinkAutomate::Generators
{
	Types::JandyExpectedMessageType GenerateMessageFromRawData(boost::circular_buffer<uint8_t>& serial_data);

}
// namespace AqualinkAutomate::Generators
