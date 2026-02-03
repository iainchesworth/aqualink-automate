#include <format>
#include <functional>

#include "devices/epump_device.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{
	EPumpDevice::EPumpDevice(std::shared_ptr<Devices::JandyDeviceType> device_id) :
		JandyDevice(device_id),
		Capabilities::Restartable(EPUMP_TIMEOUT_DURATION),
		m_RPM(std::make_pair(0, std::chrono::system_clock::now())),
		m_Watts(std::make_pair(0, std::chrono::system_clock::now()))
	{
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::EPumpMessage_Status>([this](auto&& PH1) { Slot_EPump_Status(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::EPumpMessage_RPM>([this](auto&& PH1) { Slot_EPump_RPM(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::EPumpMessage_Watts>([this](auto&& PH1) { Slot_EPump_Watts(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
	}

	void EPumpDevice::WatchdogTimeoutOccurred()
	{
	}

	EPumpDevice::TimestampedRPM EPumpDevice::ReportedRPM() const
	{
		return m_RPM;
	}

	EPumpDevice::TimestampedWatts EPumpDevice::ReportedWatts() const
	{
		return m_Watts;
	}

	void EPumpDevice::Slot_EPump_Status(const Messages::EPumpMessage_Status& msg)
	{
		LogDebug(Channel::Devices, std::format("ePump device received a EPumpMessage_Status signal. SubCommand: 0x{:02x}", msg.SubCommand()));

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void EPumpDevice::Slot_EPump_RPM(const Messages::EPumpMessage_RPM& msg)
	{
		LogDebug(Channel::Devices, std::format("ePump device received a EPumpMessage_RPM signal. RPM: {}", msg.RPM()));
		m_RPM = std::make_pair(msg.RPM(), std::chrono::system_clock::now());

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void EPumpDevice::Slot_EPump_Watts(const Messages::EPumpMessage_Watts& msg)
	{
		LogDebug(Channel::Devices, std::format("ePump device received a EPumpMessage_Watts signal. Watts: {}", msg.Watts()));
		m_Watts = std::make_pair(msg.Watts(), std::chrono::system_clock::now());

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

}
// namespace AqualinkAutomate::Devices
