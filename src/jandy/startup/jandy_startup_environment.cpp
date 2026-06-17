#include "startup/jandy_startup_environment.h"

#include <format>

#include <magic_enum/magic_enum.hpp>

#include "devices/capabilities/emulated.h"
#include "devices/iaq_device.h"
#include "devices/onetouch_device.h"
#include "devices/iaq/iaq_page_registry.h"
#include "devices/jandy_device_id.h"
#include "devices/jandy_device_types.h"
#include "devices/jandy_emulated_device_factory.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "logging/logging.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_probe.h"
#include "messages/jandy_message_status.h"
#include "messages/iaq/iaq_message_poll.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Jandy::Startup
{

	JandyStartupEnvironment::JandyStartupEnvironment(Kernel::HubLocator& hub_locator, std::chrono::seconds chlorinator_setpoint_refresh_interval)
		: m_HubLocator(hub_locator)
		, m_EquipmentHub(hub_locator.Find<Kernel::EquipmentHub>())
		, m_DataHub(hub_locator.Find<Kernel::DataHub>())
		, m_ChlorinatorSetpointRefreshInterval(chlorinator_setpoint_refresh_interval)
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

		// A configured panel actively polls an already-discovered AqualinkTouch with IAQ_Poll
		// (0x30); seeing that is as good a "0x33 is an AqualinkTouch controller" signal as the
		// cold-boot probe (and survives a capture taken after discovery).
		if (auto iaq_poll_signal = Messages::IAQMessage_Poll::GetSignal())
		{
			m_IAQPollConnection = iaq_poll_signal->connect([this](const Messages::IAQMessage_Poll& msg)
			{
				m_ProbedIds.insert(msg.Destination().Id()());
			});
		}

		// Likewise the master addresses a OneTouch with Status (0x02).
		if (auto status_signal = Messages::JandyMessage_Status::GetSignal())
		{
			m_StatusConnection = status_signal->connect([this](const Messages::JandyMessage_Status& msg)
			{
				m_ProbedIds.insert(msg.Destination().Id()());
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
			// An emulated AqualinkTouch sources data via targeted page navigation on start-up
			// (the page-push method) -- arm its survey of the data pages the pushed home page
			// does not carry (setpoints, etc.). It runs once, after the home page is established.
			if (type == Devices::JandyEmulatedDeviceTypes::IAQ)
			{
				if (auto* iaq_device = dynamic_cast<Devices::IAQDevice*>(device.get()); iaq_device != nullptr)
				{
					iaq_device->EnablePageSurvey(Devices::IAQ::DefaultAqualinkTouchRegistry());
				}
			}

			// An emulated OneTouch can proactively re-scrape the configured chlorinator setpoint
			// from the Set AquaPure menu -- arm its periodic refresh with the configured interval.
			if (type == Devices::JandyEmulatedDeviceTypes::OneTouch)
			{
				if (auto* onetouch_device = dynamic_cast<Devices::OneTouchDevice*>(device.get()); onetouch_device != nullptr)
				{
					onetouch_device->EnableChlorinatorSetpointRefresh(m_ChlorinatorSetpointRefreshInterval);
				}
			}

			// Prefer RELOCATION over suppression on a bus collision: if this instance later
			// detects a real device at its address, move our emulation to a free instance of the
			// same class instead of going silent.
			if (auto* emulated = dynamic_cast<Devices::Capabilities::Emulated*>(device.get()); emulated != nullptr)
			{
				emulated->SetCollisionHandler([this, type, id]() { return RelocateEmulation(type, id); });
			}

			m_EquipmentHub->AddDevice(std::move(device));
			m_EmulatedIds.insert(id);
		}
	}

	bool JandyStartupEnvironment::RelocateEmulation(Devices::JandyEmulatedDeviceTypes type, std::uint8_t colliding_id)
	{
		// A real device now owns `colliding_id`. Find a free instance of the SAME class (the
		// master probes 0-3 of each class, so two of a class co-exist) and stand our emulation up
		// there instead of suppressing.
		const auto device_class = Devices::JandyDeviceType(Devices::JandyDeviceId(colliding_id)).Class();
		const auto candidates = Devices::JandyDeviceType::InstanceAddressesForClass(device_class);

		std::set<std::uint8_t> taken = m_EmulatedIds;   // slots we already occupy
		taken.insert(colliding_id);                     // and the colliding (now-real) one
		for (auto occupied : OccupiedAddresses())
		{
			taken.insert(occupied);
		}

		for (auto candidate : candidates)
		{
			if (!taken.contains(candidate))
			{
				LogInfo(Channel::Devices, std::format("Startup: real device at 0x{:02x}; relocating emulated {} to free instance 0x{:02x}",
					colliding_id, magic_enum::enum_name(type), candidate));
				EmulateDevice(type, candidate, "relocated after bus collision");
				return true;
			}
		}

		LogWarning(Channel::Devices, std::format("Startup: real device at 0x{:02x} but no free instance of its class to relocate emulated {} -- staying a passive decoder",
			colliding_id, magic_enum::enum_name(type)));
		return false;
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
