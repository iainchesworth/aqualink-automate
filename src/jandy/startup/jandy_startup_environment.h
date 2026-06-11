#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <string_view>

#include <boost/signals2/connection.hpp>

#include "devices/jandy_emulated_device_types.h"
#include "startup/jandy_startup_coordinator.h"

namespace AqualinkAutomate::Kernel { class HubLocator; class EquipmentHub; class DataHub; }

namespace AqualinkAutomate::Jandy::Startup
{

	// Production IStartupEnvironment backed by the live hubs + passive bus observation.
	//
	//  - EmulateDevice  stands the device up in the EquipmentHub (shared factory).
	//  - ObservedProbes records the master's discovery probes (cmd 0x00) -- the capability signal.
	//  - OccupiedAddresses reports addresses where a REAL device answered, so the controller can
	//    be relocated off it. Presence is inferred from a device->master ACK that follows the
	//    master's probe of that address (excluding our own emulations). On a point-to-point link
	//    with no real devices this never fires, so OccupiedAddresses is empty -- exactly right,
	//    nothing to relocate around. (The per-device Emulated::SuppressEmulation remains the
	//    runtime collision backstop if two transmitters ever share an address.)
	//  - PanelModel / PanelRevision read DataHub::EquipmentVersions.
	class JandyStartupEnvironment : public IStartupEnvironment
	{
	public:
		explicit JandyStartupEnvironment(Kernel::HubLocator& hub_locator);

	public:
		void EmulateDevice(Devices::JandyEmulatedDeviceTypes type, std::uint8_t id, std::string_view role) override;
		std::set<std::uint8_t> ObservedProbes() const override;
		std::set<std::uint8_t> OccupiedAddresses() const override;
		std::string PanelModel() const override;
		std::string PanelRevision() const override;

	private:
		Kernel::HubLocator& m_HubLocator;
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
		std::shared_ptr<Kernel::DataHub> m_DataHub;

		std::set<std::uint8_t> m_ProbedIds;     // addresses the master probed (cmd 0x00)
		std::set<std::uint8_t> m_RespondedIds;  // addresses a device answered from
		std::set<std::uint8_t> m_EmulatedIds;   // our own emulations (excluded from occupied)
		std::uint8_t m_LastProbedId{ 0x00 };

		boost::signals2::scoped_connection m_ProbeConnection;
		boost::signals2::scoped_connection m_AckConnection;
	};

}
// namespace AqualinkAutomate::Jandy::Startup
