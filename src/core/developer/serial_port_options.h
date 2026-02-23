#pragma once

#include <cstdint>

#include "serial/serial_port_enums.h"

namespace AqualinkAutomate::Developer
{

	struct SerialPortOptions
	{
		uint32_t baud_rate{ 9600 };
		Serial::Parity parity{ Serial::Parity::None };
		uint8_t character_size{ 8 };
		Serial::StopBits stop_bits{ Serial::StopBits::One };
		Serial::FlowControl flow_control{ Serial::FlowControl::None };
	};

}
// namespace AqualinkAutomate::Developer
