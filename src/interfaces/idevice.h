#pragma once

#include <chrono>
#include <cstdint>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

namespace AqualinkAutomate::Interfaces
{

    class IDevice
    {
    public:
        using DeviceId = uint8_t;

    public:
        IDevice(boost::asio::io_context& io_context, const DeviceId id, std::chrono::seconds timeout_in_seconds);
        virtual ~IDevice();

    public:
        DeviceId Id() const;
        bool IsOperating() const;

    private:
        void StartWaitForTimeout();
        void TimeoutOccurred(const boost::system::error_code& ec);

    protected:
        void KickTimeoutWatchdog();

    private:
        const DeviceId m_Id;
        bool m_IsOperating;

    private:
        boost::asio::steady_timer m_TimeoutTimer;
        const std::chrono::seconds m_TimeoutDuration;
    };

}
// namespace AqualinkAutomate::Interfaces
