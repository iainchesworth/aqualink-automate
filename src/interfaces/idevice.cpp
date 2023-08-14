#include <format>
#include <functional>

#include "devices/device_status.h"
#include "interfaces/idevice.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

    IDevice::IDevice(boost::asio::io_context& io_context, std::unique_ptr<IDeviceIdentifier>&& device_id, std::chrono::seconds timeout_in_seconds = std::chrono::seconds(30)) :
        m_IsOperating(false),
        m_DeviceId(std::move(device_id)),
        m_DeviceStatus(std::make_unique<Devices::DeviceStatus_Unknown>()),
        m_TimeoutTimer(io_context),
        m_TimeoutDuration(timeout_in_seconds)
    {
        Status(Devices::DeviceStatus_Normal{});
    }

    IDevice::~IDevice()
    {
    }

    IDevice::IDevice(IDevice&& other) noexcept : 
        m_IsOperating(std::move(other.m_IsOperating)),
        m_DeviceId(std::move(other.m_DeviceId)),
        m_TimeoutTimer(std::move(other.m_TimeoutTimer)),
        m_TimeoutDuration(other.m_TimeoutDuration)
    {
    }

    bool IDevice::IsOperating() const
    {
        return m_IsOperating;
    }

    const IDeviceIdentifier& IDevice::DeviceId() const
    {
        return *m_DeviceId;
    }

    const IDeviceStatus& IDevice::Status() const
    {
        return *m_DeviceStatus;
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
