#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <nlohmann/json.hpp>

#include "devices/jandy_controller.h"
#include "devices/jandy_device_types.h"
#include "devices/capabilities/describable.h"
#include "devices/capabilities/emulated.h"
#include "devices/capabilities/restartable.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_status.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::Devices
{

	// Spa-side remotes: the "Dual Spa Switch" (2x4rem, class 0x02 -> 0x10-0x13) and the
	// "Spa Link" (8button, class 0x04 -> 0x20-0x23). They are spa-side keypads: the master
	// polls them with a cmd-0x02 LED/indicator image and the remote reports a pressed button.
	//
	// Wire shape (see docs/alwin32/spaside-remotes.md, Ghidra-verified):
	//   master -> remote : cmd 0x02, a 6-byte LED image (decodes as a JandyMessage_Status
	//                      addressed to our id; its aux-bit fields are meaningless for us).
	//   remote -> master : [0x01][0x00][button] -- a generic Ack (cmd 0x01, ack_type 0x00)
	//                      addressed to the MASTER (0x00), with the pressed button index in
	//                      the Ack's command byte (0 = none).
	//
	// Because the button report is a generic Ack to the master (no source id on the wire), we
	// attribute it by correlation: the master's poll to us fires our (dest-filtered) Status
	// slot which arms a one-shot flag; the immediately-following Ack to the master is ours.
	class SpasideRemoteDevice : public JandyController,
								public Capabilities::Restartable,
								public Capabilities::Emulated,
								public Capabilities::Describable
	{
		inline static const std::chrono::seconds SPASIDE_TIMEOUT_DURATION{ std::chrono::seconds(30) };

	public:
		SpasideRemoteDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated);
		~SpasideRemoteDevice() override = default;

	public:
		nlohmann::json DescribeDiagnostics() const override;

		// Last decoded pressed-button index (1..N); 0 means none seen since startup.
		uint8_t LastButton() const { return m_LastButton; }

		// Number of times the master has polled us (a proxy for "this remote is alive on the bus").
		uint32_t PollCount() const { return m_PollCount; }

	private:
		void ProcessControllerUpdates() override;
		void WatchdogTimeoutOccurred() override;

		// Master polled us (cmd-0x02 LED image, decoded as a Status to our id): arm the
		// one-shot correlation window for the next button-report Ack.
		void Slot_Spaside_Status(const Messages::JandyMessage_Status& msg);

		// A device->master Ack: if it is the one immediately following the master's poll to us
		// and carries ack_type 0x00 (spa-side, vs 0x80 OneTouch / 0x40 PDA), its command byte
		// is our pressed-button index.
		void Slot_Spaside_Ack(const Messages::JandyMessage_Ack& msg);

	private:
		bool m_AwaitingButtonAck{ false };
		uint8_t m_LastButton{ 0x00 };
		uint32_t m_PollCount{ 0 };
		std::optional<std::chrono::system_clock::time_point> m_LastButtonAt;
	};

}
// namespace AqualinkAutomate::Devices
