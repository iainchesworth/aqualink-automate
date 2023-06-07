#include "jandy/devices/capabilities/emulated.h"

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

}
// namespace AqualinkAutomate::Devices::Capabilities
