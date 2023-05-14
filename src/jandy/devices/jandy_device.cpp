#include "jandy/devices/jandy_device.h"

namespace AqualinkAutomate::Devices
{

	JandyDevice::JandyDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, std::chrono::seconds timeout_in_seconds, JandyDeviceOperatingModes op_mode) :
		IDevice(io_context, timeout_in_seconds),
		m_SlotManager(),
		m_OpMode(op_mode),
		m_DeviceId(device_id)
	{
	}

	JandyDevice::~JandyDevice()
	{
	}

	const Devices::JandyDeviceType& JandyDevice::DeviceId() const
	{
		return m_DeviceId;
	}

}
// namespace AqualinkAutomate::Devices

