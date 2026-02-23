#pragma once

namespace AqualinkAutomate::Interfaces
{

	class IDeviceIdentifier
	{
	public:
		IDeviceIdentifier() = default;
		virtual ~IDeviceIdentifier() = default;

	public:
		bool operator==(const IDeviceIdentifier& other) const { return Equals(other); }
		bool operator!=(const IDeviceIdentifier& other) const { return !Equals(other); }

	protected:
		virtual bool Equals(const IDeviceIdentifier& other) const = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
