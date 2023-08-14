#include "jandy/devices/jandy_device.h"

namespace AqualinkAutomate::Devices
{

	JandyDevice::JandyDevice(boost::asio::io_context& io_context, std::unique_ptr<Devices::JandyDeviceType>&& device_id, std::chrono::seconds timeout_in_seconds) :
		IDevice(io_context, std::move(device_id), timeout_in_seconds),
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

