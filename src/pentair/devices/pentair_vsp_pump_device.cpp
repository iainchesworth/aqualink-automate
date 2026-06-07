#include <format>

#include "devices/pentair_vsp_pump_device.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "messages/pump/pentair_pump_message_power.h"
#include "messages/pump/pentair_pump_message_speed.h"
#include "types/units_flow_rate.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

namespace Capabilities = AqualinkAutomate::Devices::Capabilities;

namespace AqualinkAutomate::Pentair::Devices
{

	PentairVSPPumpDevice::PentairVSPPumpDevice(const std::shared_ptr<PentairDeviceId>& device_id, Kernel::HubLocator& hub_locator) :
		PentairDevice(device_id),
		Capabilities::Restartable(PUMP_TIMEOUT_DURATION),
		m_Address((nullptr != device_id) ? (*device_id)() : static_cast<uint8_t>(0)),
		m_RPM(std::make_pair(0, std::chrono::system_clock::now())),
		m_Watts(std::make_pair(0, std::chrono::system_clock::now())),
		m_GPM(std::make_pair(0, std::chrono::system_clock::now()))
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();

		m_SlotManager.RegisterSlot<Messages::PentairPumpMessage_Status>(
			[this](const Messages::PentairPumpMessage_Status& msg) { Slot_Pump_Status(msg); });
	}

	PentairVSPPumpDevice::TimestampedRPM PentairVSPPumpDevice::ReportedRPM() const
	{
		return m_RPM;
	}

	PentairVSPPumpDevice::TimestampedWatts PentairVSPPumpDevice::ReportedWatts() const
	{
		return m_Watts;
	}

	PentairVSPPumpDevice::TimestampedGPM PentairVSPPumpDevice::ReportedGPM() const
	{
		return m_GPM;
	}

	bool PentairVSPPumpDevice::IsRunning() const
	{
		return m_IsRunning;
	}

	void PentairVSPPumpDevice::SetSpeed(uint16_t rpm) const
	{
		LogInfo(Channel::Devices, std::format("Pentair VSP pump (0x{:02x}): emitting set-speed command -> {} RPM", m_Address, rpm));
		Messages::PentairPumpMessage_Speed command(CONTROLLER_ADDRESS, m_Address, rpm);
		command.Signal_MessageToSend();
	}

	void PentairVSPPumpDevice::SetPower(bool power_on) const
	{
		LogInfo(Channel::Devices, std::format("Pentair VSP pump (0x{:02x}): emitting power command -> {}", m_Address, power_on ? "ON" : "OFF"));
		Messages::PentairPumpMessage_Power command(CONTROLLER_ADDRESS, m_Address, power_on);
		command.Signal_MessageToSend();
	}

	void PentairVSPPumpDevice::WatchdogTimeoutOccurred()
	{
		LogWarning(Channel::Devices, std::format("Pentair VSP pump (0x{:02x}) watchdog expired; marking not running", m_Address));
		m_IsRunning = false;

		if (m_DataHub)
		{
			auto pumps = m_DataHub->Pumps();
			for (auto& pump : pumps)
			{
				if (auto label = pump->AuxillaryTraits.TryGet(LabelTrait{}); label.has_value() && label.value() == std::format("Pentair Pump 0x{:02x}", m_Address))
				{
					pump->AuxillaryTraits.Set(PumpStatusTrait{}, Kernel::PumpStatuses::Off);
				}
			}
		}
	}

	void PentairVSPPumpDevice::Slot_Pump_Status(const Messages::PentairPumpMessage_Status& msg)
	{
		// Status frames are broadcast by the pump; match on the FROM address.
		if (msg.From() != m_Address)
		{
			return;
		}

		LogDebug(Channel::Devices, std::format("Pentair VSP pump (0x{:02x}) status: RPM {}, Watts {}, GPM {}, Running {}",
			m_Address, msg.RPM(), msg.Watts(), msg.GPM(), msg.IsRunning()));

		const auto now = std::chrono::system_clock::now();
		m_RPM = std::make_pair(msg.RPM(), now);
		m_Watts = std::make_pair(msg.Watts(), now);
		m_GPM = std::make_pair(msg.GPM(), now);
		m_IsRunning = msg.IsRunning();

		PushStatusToDataHub(msg);

		Capabilities::Restartable::Kick();
	}

	void PentairVSPPumpDevice::EnsurePumpDeviceExists()
	{
		if (!m_DataHub)
		{
			return;
		}

		const std::string label = std::format("Pentair Pump 0x{:02x}", m_Address);

		// Already present?
		for (const auto& pump : m_DataHub->Pumps())
		{
			if (auto existing = pump->AuxillaryTraits.TryGet(LabelTrait{}); existing.has_value() && existing.value() == label)
			{
				return;
			}
		}

		auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
		ptr->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Pump);
		ptr->AuxillaryTraits.Set(LabelTrait{}, label);
		ptr->AuxillaryTraits.Set(PumpTypeTrait{}, Kernel::PumpTypes::FilterCirculation);
		ptr->AuxillaryTraits.Set(PumpSpeedTrait{}, Kernel::PumpSpeeds::VariableSpeed);
		ptr->AuxillaryTraits.Set(PumpStatusTrait{}, Kernel::PumpStatuses::Off);

		m_DataHub->Devices.Add(std::move(ptr));
	}

	void PentairVSPPumpDevice::PushStatusToDataHub(const Messages::PentairPumpMessage_Status& msg)
	{
		if (!m_DataHub)
		{
			return;
		}

		EnsurePumpDeviceExists();

		const std::string label = std::format("Pentair Pump 0x{:02x}", m_Address);

		for (auto& pump : m_DataHub->Pumps())
		{
			auto existing = pump->AuxillaryTraits.TryGet(LabelTrait{});
			if (!existing.has_value() || existing.value() != label)
			{
				continue;
			}

			pump->AuxillaryTraits.Set(PumpStatusTrait{}, msg.IsRunning() ? Kernel::PumpStatuses::Running : Kernel::PumpStatuses::Off);
			pump->AuxillaryTraits.Set(FlowRateTrait{}, Kernel::FlowRate(static_cast<double>(msg.GPM()) * Units::gpm));
			break;
		}
	}

}
// namespace AqualinkAutomate::Pentair::Devices
