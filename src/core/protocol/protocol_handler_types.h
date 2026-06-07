#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <tuple>

#include <boost/circular_buffer.hpp>
#include <boost/system/error_code.hpp>

#include "protocol/protocol_handler_constants.h"
#include "protocol/protocol_message_types.h"
#include "serial/serial_port.h"

namespace AqualinkAutomate::Protocol
{
	using ReadOps_SerialBufferType = boost::circular_buffer<uint8_t>;
}
// namespace AqualinkAutomate::Protocol

