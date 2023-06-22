#include <format>
#include <functional>

#include "interfaces/idevice.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

    IDevice::IDevice(boost::asio::io_context& io_context, std::chrono::seconds timeout_in_seconds = std::chrono::seconds(30)) :
        m_IsOperating(false),
        m_TimeoutTimer(io_context),
        m_TimeoutDuration(timeout_in_seconds)
    {
    }

    IDevice::~IDevice()
    {
    }

    bool IDevice::IsOperating() const
    {
        return m_IsOperating;
    }

    void IDevice::StartWaitForTimeout()
    {
        m_TimeoutTimer.expires_after(m_TimeoutDuration);
        m_TimeoutTimer.async_wait(std::bind(&IDevice::TimeoutOccurred, this, std::placeholders::_1));

        // Since we've started the timer, mark this device as operating...
        m_IsOperating = true;
    }

    void IDevice::KickTimeoutWatchdog()
    {
        StartWaitForTimeout();
    }

    void IDevice::TimeoutOccurred(const boost::system::error_code& ec)
    {
        switch (ec.value())
        {
        case boost::system::errc::success:
            // There's not been a message with the <timeout duration> so mark this device as not operating...
            m_IsOperating = false;
            break;
        
        case boost::asio::error::operation_aborted:
            [[fallthrough]];
		case boost::system::errc::operation_canceled:
            // IGNORE THIS RESPONSE...  When expires_after() sets the expiry time, any pending asynchronous wait operations will be cancelled and the 
            // handler for each cancelled operation will be invoked with the boost::asio::error::operation_aborted error code.
            break;

        default:
            LogDebug(Channel::Devices, std::format("Device timeout; timer async_wait failed; error -> {}, message -> {}", ec.value(), ec.message()));
            break;
		}
    }

}
// namespace AqualinkAutomate::Interfaces
