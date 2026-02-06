#include <format>

#include <boost/system/error_code.hpp>

#include "logging/logging.h"
#include "options/options_serial_options.h"
#include "protocol/protocol_handler_constants.h"
#include "serial/serial_initialise.h"
#include "serial/serial_port_enums.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Serial
{

	void Initialise(Options::Settings& settings, const std::shared_ptr<Serial::SerialPort>& serial_port)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial::Initialise", std::source_location::current());

		auto serial_settings_result = settings.Get<Options::Serial::SerialSettings>();
		if (!serial_settings_result)
		{
			LogWarning(Channel::Serial, "Serial settings not found");
			return;
		}

		const auto& serial_settings = serial_settings_result.value().get();

		if (nullptr == serial_port)
		{
			LogWarning(Channel::Serial, "Attempted to configure a serial port object that has not been created");
			return;
		}

		if (serial_port->is_open())
		{
			LogDebug(Channel::Serial, "Attempted to initialise an already open serial port device");
			return;
		}

		boost::system::error_code ec;

		if (serial_settings.UsingPhysicalSerialPort())
		{
			auto open_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial::Initialise -> open_physical_port", std::source_location::current());

			LogDebug(Channel::Serial, std::format("Attempting to open serial device: {}", serial_settings.serial_port));
			serial_port->open(serial_settings.serial_port, ec);

			if (ec)
			{
				LogFatal(Channel::Serial, std::format("Failed to open physical serial port: {}", serial_settings.serial_port));
				return;
			}
		}
		else if (serial_settings.UsingRemoteSerialPort())
		{
			auto open_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial::Initialise -> open_remote_port", std::source_location::current());

			LogDebug(Channel::Serial, std::format("Attempting to open remote serial device: {}", serial_settings.remote_serial_port));
			serial_port->open(serial_settings.remote_serial_port, ec);

			if (ec)
			{
				LogFatal(Channel::Serial, std::format("Failed to open remote serial port: {}", serial_settings.remote_serial_port));
				return;
			}
		}

		auto config_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial::Initialise -> configure_options", std::source_location::current());

		LogDebug(Channel::Serial, std::format("Configuring serial port device: {}", serial_settings.serial_port));
		serial_port->set_baud_rate(9600, ec);
		serial_port->set_parity(Serial::Parity::None, ec);
		serial_port->set_character_size(8, ec);
		serial_port->set_stop_bits(Serial::StopBits::One, ec);
		serial_port->set_flow_control(Serial::FlowControl::None, ec);
		serial_port->set_read_timeout(Protocol::Constants::SERIAL_READ_TIMEOUT_DURATION, ec);
	}

}
// namespace AqualinkAutomate::Serial
