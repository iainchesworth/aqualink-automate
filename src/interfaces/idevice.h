#pragma once

#include <memory>

#include "interfaces/ideviceidentifier.h"

namespace AqualinkAutomate::Interfaces
{

    class IDevice
    {
    public:
        IDevice(std::shared_ptr<IDeviceIdentifier> device_id);
        virtual ~IDevice();

    public:
        IDevice(const IDevice& other) = delete;
        IDevice(IDevice&& other) noexcept;

    public:
        const IDeviceIdentifier& DeviceId() const;

    private:
        std::shared_ptr<IDeviceIdentifier> m_DeviceId;
    };

}
// namespace AqualinkAutomate::Interfaces
