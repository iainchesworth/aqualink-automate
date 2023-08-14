#pragma once

namespace AqualinkAutomate::Interfaces
{

	class IDeviceIdentifier
	{
	public:
		IDeviceIdentifier() = default;
		virtual ~IDeviceIdentifier() = default;

	public:
		virtual bool operator==(const IDeviceIdentifier& other) const = 0;
		virtual bool operator!=(const IDeviceIdentifier& other) const = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
