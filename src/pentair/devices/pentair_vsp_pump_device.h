#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

#include "devices/capabilities/restartable.h"
#include "devices/pentair_device.h"
#include "devices/pentair_device_id.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "messages/pump/pentair_pump_message_status.h"

namespace AqualinkAutomate::Pentair::Devices
{

	// Handler for a Pentair variable-speed pump (IntelliFlo, addresses 0x60-0x6F).
	//
	// Decodes the pump's status broadcasts (RPM / watts / GPM), keeps a watchdog
	// alive while the pump is reporting, surfaces the pump as a Pump auxillary in
	// the DataHub, and can emit Speed (0x01) and Power (0x06) commands via the
	// message send-signals.
	//
	// Messages are matched on their FROM address inside the status slot (status
	// frames are broadcast pump -> controller, so FROM is the pump address).
	class PentairVSPPumpDevice : public PentairDevice, public AqualinkAutomate::Devices::Capabilities::Restartable
	{
		inline static const std::chrono::seconds PUMP_TIMEOUT_DURATION{ std::chrono::seconds(30) };

		// Controller / master address used as the FROM of emitted commands.
		static constexpr uint8_t CONTROLLER_ADDRESS = 0x10;

	public:
		using TimestampedRPM = std::pair<uint16_t, std::chrono::time_point<std::chrono::system_clock>>;
		using TimestampedWatts = std::pair<uint16_t, std::chrono::time_point<std::chrono::system_clock>>;
		using TimestampedGPM = std::pair<uint8_t, std::chrono::time_point<std::chrono::system_clock>>;

	public:
		PentairVSPPumpDevice(const std::shared_ptr<PentairDeviceId>& device_id, Kernel::HubLocator& hub_locator);
		~PentairVSPPumpDevice() override = default;

	public:
		TimestampedRPM ReportedRPM() const;
		TimestampedWatts ReportedWatts() const;
		TimestampedGPM ReportedGPM() const;
		bool IsRunning() const;

	public:
		// Emit a set-speed command (controller -> this pump).
		void SetSpeed(uint16_t rpm) const;

		// Emit a power on/off command (controller -> this pump).
		void SetPower(bool power_on) const;

	private:
		void WatchdogTimeoutOccurred() override;

	private:
		void Slot_Pump_Status(const Messages::PentairPumpMessage_Status& msg);

	private:
		void EnsurePumpDeviceExists();
		void PushStatusToDataHub(const Messages::PentairPumpMessage_Status& msg);

	private:
		uint8_t m_Address;
		TimestampedRPM m_RPM;
		TimestampedWatts m_Watts;
		TimestampedGPM m_GPM;
		bool m_IsRunning{ false };
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::Pentair::Devices
