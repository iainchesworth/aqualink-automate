#include <termios.h>

#include "serial/port_types/physical_serial_port_impl.h"

namespace AqualinkAutomate::Serial::PortTypes
{

    void PhysicalSerialPortImpl::set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec)
    {
        auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PhysicalSerialPortImpl::set_read_timeout", std::source_location::current());

        auto native_handle = m_SerialPort.native_handle();
        auto timeout_ms = static_cast<unsigned long>(timeout.count());

        struct termios tio;
        if (tcgetattr(native_handle, &tio) != 0)
        {
            ec = boost::system::error_code(errno, boost::system::system_category());
            return;
        }

        // VTIME is in tenths of a second; clamp to at least 1 decisecond
        auto deciseconds = timeout_ms / 100;
        if (deciseconds == 0 && timeout_ms > 0) deciseconds = 1;
        tio.c_cc[VTIME] = static_cast<cc_t>(deciseconds);
        tio.c_cc[VMIN] = 0;

        if (tcsetattr(native_handle, TCSANOW, &tio) != 0)
        {
            ec = boost::system::error_code(errno, boost::system::system_category());
        }
        else
        {
            ec = {};
        }
    }

}
// namespace AqualinkAutomate::Serial::PortTypes
