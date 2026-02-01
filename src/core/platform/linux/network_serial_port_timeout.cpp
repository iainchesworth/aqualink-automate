#include <sys/socket.h>

#include "logging/logging.h"
#include "serial/port_types/network_serial_port_impl.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Serial::PortTypes
{

    void NetworkSerialPortImpl::set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec)
    {
        auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl -> set_read_timeout", std::source_location::current());

        if (!is_open())
        {
            LogWarning(Channel::Serial, "Attempted to set read timeout on closed network serial port");
            ec = boost::asio::error::bad_descriptor;
            return;
        }

        LogDebug(Channel::Serial, std::format("Setting read timeout: {}ms", timeout.count()));

        struct timeval tv;
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(timeout.count() / 1000);
        tv.tv_usec = static_cast<decltype(tv.tv_usec)>((timeout.count() % 1000) * 1000);
        setsockopt(m_Socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&tv), sizeof(tv));

        ec = {};
    }

}
// namespace AqualinkAutomate::Serial::PortTypes
