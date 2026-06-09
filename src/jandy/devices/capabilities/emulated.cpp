#include "devices/capabilities/emulated.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	Emulated::Emulated(bool is_emulated) :
		m_IsEmulated(is_emulated)
	{
	}

	bool Emulated::IsEmulated() const
	{
		return m_IsEmulated;
	}

	void Emulated::SuppressEmulation()
	{
		// One-way latch: once suppressed, stay suppressed for the device lifetime.
		m_EmulationSuppressed = true;
	}

	bool Emulated::IsEmulationSuppressed() const
	{
		return m_EmulationSuppressed;
	}

	bool Emulated::IsEmulationActive() const
	{
		return m_IsEmulated && !m_EmulationSuppressed;
	}

}
// namespace AqualinkAutomate::Devices::Capabilities
