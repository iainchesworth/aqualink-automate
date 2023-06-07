#include "jandy/devices/jandy_device_id.h"

namespace AqualinkAutomate::Devices
{

	JandyDeviceId::JandyDeviceId(value_type device_id) : 
		m_DeviceId(device_id)
	{
	}

	JandyDeviceId::value_type JandyDeviceId::operator()() const
	{
		return m_DeviceId;
	}

	bool JandyDeviceId::operator==(const JandyDeviceId& other) const
	{
		return (m_DeviceId == other.m_DeviceId);
	}

	bool JandyDeviceId::operator!=(const JandyDeviceId& other) const
	{
		return !operator==(other);
	}

}
// namespace AqualinkAutomate::Devices;
