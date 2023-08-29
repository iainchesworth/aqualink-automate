#include "devices/device_status.h"
#include "jandy/devices/jandy_device.h"

namespace AqualinkAutomate::Devices
{

	JandyDevice::JandyDevice(std::shared_ptr<Devices::JandyDeviceType> device_id) :
		IDevice(device_id),
		IStatusPublisher(DeviceStatus_Unknown{}),
		m_SlotManager()
	{
	}

	JandyDevice::~JandyDevice()
	{
	}

	const Devices::JandyDeviceType& JandyDevice::DeviceId() const
	{
		return dynamic_cast<const Devices::JandyDeviceType&>(IDevice::DeviceId());
	}

}
// namespace AqualinkAutomate::Devices

