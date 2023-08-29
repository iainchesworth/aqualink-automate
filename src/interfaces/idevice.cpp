#include "interfaces/idevice.h"

namespace AqualinkAutomate::Interfaces
{

    IDevice::IDevice(std::shared_ptr<IDeviceIdentifier> device_id) :
        m_DeviceId(std::move(device_id))
    {
    }

    IDevice::~IDevice()
    {
    }

    IDevice::IDevice(IDevice&& other) noexcept : 
        m_DeviceId(std::move(other.m_DeviceId))
    {
    }

    const IDeviceIdentifier& IDevice::DeviceId() const
    {
        return *m_DeviceId;
    }

}
// namespace AqualinkAutomate::Interfaces
