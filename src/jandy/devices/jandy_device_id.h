#pragma once

#include <cstdint>
#include <functional>

namespace AqualinkAutomate::Devices
{

	class JandyDeviceId
	{
	public:
		using value_type = uint8_t;

	public:
		JandyDeviceId(value_type device_id);

	public:
		value_type operator()() const;

	public:
		bool operator==(const JandyDeviceId& other) const;
		bool operator!=(const JandyDeviceId& other) const;

	private:
		value_type m_DeviceId;
	};

}
// namespace AqualinkAutomate::Devices

namespace std
{

	template<>
	struct std::hash<AqualinkAutomate::Devices::JandyDeviceId>
	{
		std::size_t operator()(AqualinkAutomate::Devices::JandyDeviceId const& device_id) const noexcept
		{
			return std::hash<uint8_t>{}(device_id());
		}
	};

}
// namespace std
