#include <WinSock2.h>

#include <boost/asio/error.hpp>

#include "logging/logging.h"
#include "serial/port_types/network_serial_port_impl.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Serial::PortTypes
{

    void NetworkSerialPortImpl::set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec)
    {
        auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::set_read_timeout", std::source_location::current());

        if (!is_open())
        {
            LogWarning(Channel::Serial, "Attempted to set read timeout on closed network serial port");
            ec = boost::asio::error::bad_descriptor;
            return;
        }

        LogDebug(Channel::Serial, std::format("Setting read timeout: {}ms", timeout.count()));

        const DWORD timeout_ms = static_cast<DWORD>(timeout.count());

        if (SOCKET_ERROR == setsockopt(m_Socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)))
        {
            const int wsa_error = ::WSAGetLastError();
            LogWarning(Channel::Serial, std::format("Failed to set socket read timeout; WSAGetLastError() -> {}", wsa_error));
            ec.assign(wsa_error, boost::asio::error::get_system_category());
            return;
        }

        ec = {};
    }

}
// namespace AqualinkAutomate::Serial::PortTypes
