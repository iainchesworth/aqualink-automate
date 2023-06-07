#include "jandy/devices/jandy_device_types.h"

namespace AqualinkAutomate::Devices
{

	JandyDeviceType::JandyDeviceType() :
		m_DeviceClass(DeviceClasses::Unknown),
		m_DeviceId(0xFF)
	{
	}

	JandyDeviceType::JandyDeviceType(DeviceId device_id) :
		m_DeviceClass(DeviceClasses::Unknown),
		m_DeviceId(device_id)
	{
		for (auto& device_class : m_KnownDeviceIdsList)
		{
			for (auto& id : device_class.second)
			{
				if (static_cast<DeviceId>(id) == device_id)
				{
					m_DeviceClass = device_class.first;
				}
			}
		}
	}

	JandyDeviceType::JandyDeviceType(const JandyDeviceType& other) :
		m_DeviceClass(other.m_DeviceClass),
		m_DeviceId(other.m_DeviceId)
	{
	}

	JandyDeviceType& JandyDeviceType::operator=(const JandyDeviceType& other)
	{
		if (this != &other)
		{
			// "Copy" the data to this instance.
			m_DeviceClass = other.m_DeviceClass;
			m_DeviceId = other.m_DeviceId;
		}

		return *this;
	}

	JandyDeviceType::JandyDeviceType(JandyDeviceType&& other) noexcept :
		m_DeviceClass(std::move(other.m_DeviceClass)),
		m_DeviceId(std::move(other.m_DeviceId))
	{
		// Reset the other instance to default "unknown" values.
		other.m_DeviceClass = DeviceClasses::Unknown;
		other.m_DeviceId = 0xFF;
	}

	JandyDeviceType& JandyDeviceType::operator=(JandyDeviceType&& other) noexcept
	{
		if (this != &other)
		{
			// "Move" the data to this instance.
			m_DeviceClass = std::move(other.m_DeviceClass);
			m_DeviceId = std::move(other.m_DeviceId);

			// Reset the other instance to default "unknown" values.
			other.m_DeviceClass = DeviceClasses::Unknown;
			other.m_DeviceId = 0xFF;
		}

		return *this;
	}

	bool JandyDeviceType::operator==(const JandyDeviceType& other) const 
	{
		return m_DeviceId == other.m_DeviceId;
	}

	bool JandyDeviceType::operator!=(const JandyDeviceType& other) const
	{
		return !(*this == other);
	}

	JandyDeviceType::DeviceId JandyDeviceType::operator()() const
	{
		return m_DeviceId;
	}

	DeviceClasses JandyDeviceType::Class() const
	{
		return m_DeviceClass;
	}

	JandyDeviceType::DeviceId JandyDeviceType::Id() const
	{
		return m_DeviceId;
	}

}
// namespace AqualinkAutomate::Devices
