#pragma once

#include <string_view>

namespace AqualinkAutomate::Interfaces
{

	class IDeviceStatus
	{
	public:
		IDeviceStatus() = default;
		virtual ~IDeviceStatus() = default;

	public:
		virtual std::string_view Name() const = 0;

	private:
		friend bool operator==(const IDeviceStatus& lhs, const IDeviceStatus& rhs);
		friend bool operator!=(const IDeviceStatus& lhs, const IDeviceStatus& rhs);
	};

	bool operator==(const IDeviceStatus& lhs, const IDeviceStatus& rhs);
	bool operator!=(const IDeviceStatus& lhs, const IDeviceStatus& rhs);

}
// namespace AqualinkAutomate::Interfaces
