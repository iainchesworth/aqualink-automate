#include "startup/jandy_startup_environment.h"

#include <format>

#include <magic_enum/magic_enum.hpp>

#include "devices/jandy_device_id.h"
#include "devices/jandy_emulated_device_factory.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "logging/logging.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_probe.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Jandy::Startup
{

	JandyStartupEnvironment::JandyStartupEnvironment(Kernel::HubLocator& hub_locator)
		: m_HubLocator(hub_locator)
		, m_EquipmentHub(hub_locator.Find<Kernel::EquipmentHub>())
		, m_DataHub(hub_locator.Find<Kernel::DataHub>())
	{
		// Observe the master's discovery probes (cmd 0x00) -- the controller-type signal.
		if (auto probe_signal = Messages::JandyMessage_Probe::GetSignal())
		{
			m_ProbeConnection = probe_signal->connect([this](const Messages::JandyMessage_Probe& msg)
			{
				const auto id = msg.Destination().Id()();
				m_ProbedIds.insert(id);
				m_LastProbedId = id;
			});
		}

		// Presence: a device->master ACK (dest 0x00) immediately after a probe means a real
		// device answered that probe. (Requires master-bound frames to be decoded; with no real
		// devices on the link this never fires, so OccupiedAddresses stays empty.)
		if (auto ack_signal = Messages::JandyMessage_Ack::GetSignal())
		{
			m_AckConnection = ack_signal->connect([this](const Messages::JandyMessage_Ack& msg)
			{
				if ((msg.Destination().Id()() == 0x00) && (m_LastProbedId != 0x00))
				{
					m_RespondedIds.insert(m_LastProbedId);
				}
			});
		}
	}

	void JandyStartupEnvironment::EmulateDevice(Devices::JandyEmulatedDeviceTypes type, std::uint8_t id, std::string_view role)
	{
		if (m_EquipmentHub == nullptr)
		{
			LogError(Channel::Devices, "Startup: no EquipmentHub; cannot stand up emulated device");
			return;
		}

		LogInfo(Channel::Devices, std::format("Startup: emulating {} at 0x{:02x} ({})", magic_enum::enum_name(type), id, role));

		if (auto device = Devices::MakeEmulatedDevice(type, Devices::JandyDeviceId(id), m_HubLocator); device != nullptr)
		{
			m_EquipmentHub->AddDevice(std::move(device));
			m_EmulatedIds.insert(id);
		}
	}

	std::set<std::uint8_t> JandyStartupEnvironment::ObservedProbes() const
	{
		return m_ProbedIds;
	}

	std::set<std::uint8_t> JandyStartupEnvironment::OccupiedAddresses() const
	{
		std::set<std::uint8_t> occupied;
		for (auto id : m_RespondedIds)
		{
			if (!m_EmulatedIds.contains(id))   // never count our own emulations as "occupied"
			{
				occupied.insert(id);
			}
		}
		return occupied;
	}

	std::string JandyStartupEnvironment::PanelModel() const
	{
		return (m_DataHub != nullptr) ? m_DataHub->EquipmentVersions.ModelNumber() : std::string{};
	}

	std::string JandyStartupEnvironment::PanelRevision() const
	{
		return (m_DataHub != nullptr) ? m_DataHub->EquipmentVersions.FirmwareRevision() : std::string{};
	}

}
// namespace AqualinkAutomate::Jandy::Startup
