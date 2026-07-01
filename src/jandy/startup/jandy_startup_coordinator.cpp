#include "startup/jandy_startup_coordinator.h"

#include <format>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "startup/jandy_startup_planner.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Jandy::Startup
{
	StartupCoordinator::StartupCoordinator(IStartupEnvironment& env)
		: m_Env(env)
	{
	}

	void StartupCoordinator::Begin()
	{
		// Phase 1: stand up the SerialAdapter. It answers the master immediately (keeping the
		// bus healthy), sources the panel model, and is the command channel -- and it does NOT
		// depend on the controller type, so it is always safe to emulate first. If a real
		// adapter is present it self-suppresses (Emulated::SuppressEmulation).
		LogInfo(Channel::Devices, "Startup: emulating SerialAdapter and observing the master's probe set to classify the controller");
		m_Env.EmulateDevice(Devices::JandyEmulatedDeviceTypes::SerialAdapter, StartupPlanner::SERIALADAPTER_IDS.front(), "panel model + command channel (detector)");
		m_Phase = Phase::Detecting;
	}

	bool StartupCoordinator::HaveControllerSignal() const
	{
		auto profile = StartupPlanner::DeriveProfile(m_Env.ObservedProbes(), m_Env.PanelModel(), m_Env.PanelRevision());

		if (profile.probes_aqualinktouch || profile.probes_onetouch || profile.probes_pda)
		{
			return true;
		}

		// The revision (sourced via the SerialAdapter, often before the master's probe cycle is
		// fully observed) can classify a touch-capable panel early -- Rev Q+ implies the page
		// protocol -- so we need not wait out the whole detection window.
		return profile.revision_caps.is_known && profile.revision_caps.aqualink_touch;
	}

	StartupCoordinator::Phase StartupCoordinator::Advance(bool detection_window_elapsed)
	{
		switch (m_Phase)
		{
		case Phase::Idle:
			break;

		case Phase::Detecting:
		{
			// Hold in Detecting until we can classify the controller, or the driver tells us
			// the detection window has elapsed (then plan from whatever we have).
			if (!HaveControllerSignal() && !detection_window_elapsed)
			{
				break;
			}

			auto profile = StartupPlanner::DeriveProfile(m_Env.ObservedProbes(), m_Env.PanelModel(), m_Env.PanelRevision());
			m_Plan = StartupPlanner::Plan(profile);
			StartupPlanner::ResolveAddresses(m_Plan, m_Env.OccupiedAddresses());

			LogInfo(Channel::Devices, std::format("Startup: classified controller -> method={} ({})",
				magic_enum::enum_name(m_Plan.method), m_Plan.rationale));

			m_Phase = Phase::Engaging;
		}
		[[fallthrough]];

		case Phase::Engaging:
		{
			// Stand up the resolved controller emulation. The SerialAdapter is already up from
			// Begin(), so skip it here; ResolveAddresses still claimed its slot so the
			// controller never lands on it.
			for (const auto& device : m_Plan.devices)
			{
				if (device.type == Devices::JandyEmulatedDeviceTypes::SerialAdapter)
				{
					continue;
				}

				if (!device.resolved)
				{
					LogWarning(Channel::Devices, std::format("Startup: no free bus address for emulated {} -- skipping (real device occupies every candidate slot)",
						magic_enum::enum_name(device.type)));
					continue;
				}

				LogInfo(Channel::Devices, std::format("Startup: emulating {} at 0x{:02x} ({})",
					magic_enum::enum_name(device.type), device.selected_id, device.role));
				m_Env.EmulateDevice(device.type, device.selected_id, device.role);
			}

			m_Phase = Phase::Running;
			break;
		}

		case Phase::Running:
			break;
		}

		return m_Phase;
	}

}
// namespace AqualinkAutomate::Jandy::Startup
