#include "devices/pentair_device_id.h"

namespace AqualinkAutomate::Pentair::Devices
{

	PentairDeviceId::PentairDeviceId(uint8_t address) :
		m_Address(address)
	{
	}

	uint8_t PentairDeviceId::Address() const
	{
		return m_Address;
	}

	uint8_t PentairDeviceId::operator()() const
	{
		return m_Address;
	}

	bool PentairDeviceId::Equals(const Interfaces::IDeviceIdentifier& other) const
	{
		const auto* other_id = dynamic_cast<const PentairDeviceId*>(&other);
		return (nullptr != other_id) && (m_Address == other_id->m_Address);
	}

}
// namespace AqualinkAutomate::Pentair::Devices
