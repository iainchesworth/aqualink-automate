#pragma once

#include <memory>

#include "options/options_settings.h"
#include "serial/serial_port.h"

namespace AqualinkAutomate::Serial
{
	
	void Initialise(Options::Settings& settings, std::shared_ptr<Serial::SerialPort> serial_port);

}
// namespace AqualinkAutomate::Serial
