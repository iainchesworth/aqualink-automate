#include "devices/capabilities/emulated.h"

#include <format>
#include <utility>

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

	void Emulated::SetCollisionHandler(CollisionHandler handler)
	{
		m_CollisionHandler = std::move(handler);
	}

	void Emulated::HandleEmulationCollision()
	{
		// A non-emulated or already-suppressed instance has nothing to do.
		if (!m_IsEmulated || m_EmulationSuppressed)
		{
			return;
		}

		// Prefer RELOCATION over suppression: ask the address manager to stand the emulation up at
		// a free instance of the same class. Multiple of a class co-exist on the bus at different
		// instances, so moving keeps us actively emulating instead of going silent.
		const bool relocated = (m_CollisionHandler != nullptr) && m_CollisionHandler();

		// Either way THIS instance stops transmitting -- the relocated emulation (or the real
		// device) now owns this address. The read path stays live (passive decode).
		SuppressEmulation();

		LogNotify(Channel::Devices, std::format("Emulation collision on this bus address: {}; this instance is now a passive decoder.",
			relocated ? "relocated to a free instance of the same class" : "suppressed (no relocation handler / no free instance)"));
	}

}
// namespace AqualinkAutomate::Devices::Capabilities
