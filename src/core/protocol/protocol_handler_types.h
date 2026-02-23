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
	// Generic message type for protocol handling
	using ReadOps_MessageType = ExpectedProtocolMessage;

	using ReadOps_RawDataType = std::array<uint8_t, Constants::SERIAL_READ_CHUNK_SIZE>;
	using ReadOps_RawDataRetType = std::expected<std::tuple<ReadOps_RawDataType, std::size_t>, boost::system::error_code>;

	using ReadOps_SerialBufferType = boost::circular_buffer<uint8_t>;
	using ReadOps_SerialBufferRefType = std::reference_wrapper<ReadOps_SerialBufferType>;

	using Protocol_SerialPortType = Serial::SerialPort;
	using Protocol_SerialPortRefType = std::reference_wrapper<Protocol_SerialPortType>;
}
// namespace AqualinkAutomate::Protocol

