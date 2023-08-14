#pragma once

#include <chrono>
#include <concepts>
#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include "interfaces/ideviceidentifier.h"
#include "interfaces/idevicestatus.h"

namespace AqualinkAutomate::Interfaces
{

    class IDevice
    {
    public:
        IDevice(boost::asio::io_context& io_context, std::unique_ptr<IDeviceIdentifier>&& device_id, std::chrono::seconds timeout_in_seconds);
        virtual ~IDevice();

    public:
        IDevice(const IDevice& other) = delete;
        IDevice(IDevice&& other) noexcept;

    public:
        bool IsOperating() const;

    private:
        bool m_IsOperating;

    public:
        const IDeviceIdentifier& DeviceId() const;

    private:
        std::unique_ptr<IDeviceIdentifier> m_DeviceId;

    public:
        const IDeviceStatus& Status() const;

    public:
        template<typename DEVICE_STATUS>
        requires std::derived_from<DEVICE_STATUS, IDeviceStatus>
        void Status(DEVICE_STATUS&& status)
        {
            m_DeviceStatus = std::make_unique<DEVICE_STATUS>(std::move(status));
        }

    private:
        std::unique_ptr<IDeviceStatus> m_DeviceStatus;

    protected:
        void KickTimeoutWatchdog();

    private:
        void StartWaitForTimeout();
        void TimeoutOccurred(const boost::system::error_code& ec);

    private:
        boost::asio::steady_timer m_TimeoutTimer;
        const std::chrono::seconds m_TimeoutDuration;
    };

}
// namespace AqualinkAutomate::Interfaces
