#include "jandy/devices/jandy_device_types.h"

namespace AqualinkAutomate::Devices
{

	JandyDeviceType::JandyDeviceType()
	{
	}

	JandyDeviceType::JandyDeviceType(uint8_t device_id) :
		m_DeviceClass(DeviceClasses::Unknown),
		m_RawId(device_id)
	{
		for (auto& device_class : m_KnownDeviceIdsList)
		{
			for (auto& id : device_class.second)
			{
				if (static_cast<uint8_t>(id) == device_id)
				{
					m_DeviceClass = device_class.first;
				}
			}
		}
	}

	JandyDeviceType::JandyDeviceType(const JandyDeviceType& other) :
		m_DeviceClass(other.m_DeviceClass),
		m_RawId(other.m_RawId)
	{
	}

	JandyDeviceType& JandyDeviceType::operator=(const JandyDeviceType& other)
	{
		if (this != &other)
		{
			// "Copy" the data to this instance.
			m_DeviceClass = other.m_DeviceClass;
			m_RawId = other.m_RawId;
		}

		return *this;
	}

	JandyDeviceType::JandyDeviceType(JandyDeviceType&& other) noexcept :
		m_DeviceClass(std::move(other.m_DeviceClass)),
		m_RawId(std::move(other.m_RawId))
	{
		// Reset the other instance to default "unknown" values.
		other.m_DeviceClass = DeviceClasses::Unknown;
		other.m_RawId = 0xFF;
	}

	JandyDeviceType& JandyDeviceType::operator=(JandyDeviceType&& other) noexcept
	{
		if (this != &other)
		{
			// "Move" the data to this instance.
			m_DeviceClass = std::move(other.m_DeviceClass);
			m_RawId = std::move(other.m_RawId);

			// Reset the other instance to default "unknown" values.
			other.m_DeviceClass = DeviceClasses::Unknown;
			other.m_RawId = 0xFF;
		}

		return *this;
	}

	bool JandyDeviceType::operator==(const JandyDeviceType& other) const 
	{
		return m_RawId == other.m_RawId;
	}

	bool JandyDeviceType::operator!=(const JandyDeviceType& other) const
	{
		return !(*this == other);
	}

	uint8_t JandyDeviceType::operator()() const
	{
		return m_RawId;
	}

	DeviceClasses JandyDeviceType::Class() const
	{
		return m_DeviceClass;
	}

	uint8_t JandyDeviceType::Raw() const
	{
		return m_RawId;
	}

}
// namespace AqualinkAutomate::Devices
