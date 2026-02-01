#include <Windows.h>

#include "serial/port_types/physical_serial_port_impl.h"

namespace AqualinkAutomate::Serial::PortTypes
{

    void PhysicalSerialPortImpl::set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec)
    {
        auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PhysicalSerialPortImpl -> set_read_timeout", std::source_location::current());

        auto native_handle = m_SerialPort.native_handle();
        auto timeout_ms = static_cast<unsigned long>(timeout.count());

        COMMTIMEOUTS timeouts{};

        if (timeout_ms == 0)
        {
            // Non-blocking: return immediately with whatever data is available.
            // This is the Windows COMMTIMEOUTS special case for non-blocking reads.
            timeouts.ReadIntervalTimeout = MAXDWORD;
            timeouts.ReadTotalTimeoutMultiplier = 0;
            timeouts.ReadTotalTimeoutConstant = 0;
        }
        else
        {
            timeouts.ReadIntervalTimeout = 0;
            timeouts.ReadTotalTimeoutMultiplier = 0;
            timeouts.ReadTotalTimeoutConstant = static_cast<DWORD>(timeout_ms);
        }

        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts(native_handle, &timeouts))
        {
            ec = boost::system::error_code(static_cast<int>(GetLastError()), boost::system::system_category());
        }
        else
        {
            ec = {};
        }
    }

}
// namespace AqualinkAutomate::Serial::PortTypes
