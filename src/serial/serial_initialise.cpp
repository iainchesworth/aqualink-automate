#include <format>

#include <boost/system/error_code.hpp>

#include "logging/logging.h"
#include "serial/serial_initialise.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Options;

namespace AqualinkAutomate::Serial
{

	void Initialise(Options::Settings& settings, std::shared_ptr<Serial::SerialPort> serial_port)
	{
		if (nullptr == serial_port)
		{
			LogWarning(Channel::Serial, "Attempted to configure a serial port object that has not been created");
		}
		else if (serial_port->is_open())
		{
			LogDebug(Channel::Serial, "Attempted to initialise an already open serial port device");
		}
		else
		{
			boost::system::error_code ec;

			LogDebug(Channel::Serial, std::format("Attempting to open serial device: {}", settings.serial.serial_port));
			serial_port->open(settings.serial.serial_port);

			LogDebug(Channel::Serial, std::format("Configuring serial device: {}", settings.serial.serial_port));
			serial_port->set_option(boost::asio::serial_port::baud_rate(9600), ec);
			serial_port->set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none), ec);
			serial_port->set_option(boost::asio::serial_port::character_size(8), ec);
			serial_port->set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one), ec);
			serial_port->set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none), ec);
		}
	}

}
// namespace AqualinkAutomate::Serial
