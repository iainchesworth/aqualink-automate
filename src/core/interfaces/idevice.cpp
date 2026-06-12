#include "interfaces/idevice.h"

namespace AqualinkAutomate::Interfaces
{

    IDevice::IDevice(std::shared_ptr<IDeviceIdentifier> device_id) :
        m_DeviceId(std::move(device_id))
    {
    }

    const IDeviceIdentifier& IDevice::DeviceId() const
    {
        return *m_DeviceId;
    }

}
// namespace AqualinkAutomate::Interfaces
