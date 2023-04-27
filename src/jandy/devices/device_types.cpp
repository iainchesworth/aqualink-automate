#include "jandy/devices/device_types.h"

namespace AqualinkAutomate::Devices
{

	DeviceType::DeviceType()
	{
	}

	DeviceType::DeviceType(uint8_t device_id) :
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

	DeviceType::DeviceType(const DeviceType& other) :
		m_DeviceClass(other.m_DeviceClass),
		m_RawId(other.m_RawId)
	{
	}

	DeviceType& DeviceType::operator=(const DeviceType& other)
	{
		if (this != &other)
		{
			// "Copy" the data to this instance.
			m_DeviceClass = other.m_DeviceClass;
			m_RawId = other.m_RawId;
		}

		return *this;
	}

	DeviceType::DeviceType(DeviceType&& other) noexcept :
		m_DeviceClass(std::move(other.m_DeviceClass)),
		m_RawId(std::move(other.m_RawId))
	{
		// Reset the other instance to default "unknown" values.
		other.m_DeviceClass = DeviceClasses::Unknown;
		other.m_RawId = 0xFF;
	}

	DeviceType& DeviceType::operator=(DeviceType&& other) noexcept
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

	DeviceClasses DeviceType::Class() const
	{
		return m_DeviceClass;
	}

	uint8_t DeviceType::Raw() const
	{
		return m_RawId;
	}

}
// namespace AqualinkAutomate::Devices
