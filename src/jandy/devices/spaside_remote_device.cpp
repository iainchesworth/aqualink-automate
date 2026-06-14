#include <format>
#include <functional>

#include <nlohmann/json.hpp>

#include "logging/logging.h"
#include "devices/spaside_remote_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	SpasideRemoteDevice::SpasideRemoteDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated) :
		JandyController(device_id, hub_locator),
		Capabilities::Restartable(SPASIDE_TIMEOUT_DURATION),
		Capabilities::Emulated(is_emulated)
	{
		// The master's cmd-0x02 LED-image poll decodes as a Status addressed to our id; use it
		// purely as the "we were just polled" trigger that arms the button-Ack correlation.
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Status>(
			std::bind(&SpasideRemoteDevice::Slot_Spaside_Status, this, std::placeholders::_1), (*device_id)());

		// Button presses are reported as a generic Ack (cmd 0x01) addressed to the MASTER
		// (0x00), so we CANNOT filter by our own id -- register unfiltered and correlate.
		m_SlotManager.RegisterSlot<Messages::JandyMessage_Ack>(
			std::bind(&SpasideRemoteDevice::Slot_Spaside_Ack, this, std::placeholders::_1));
	}

	void SpasideRemoteDevice::ProcessControllerUpdates()
	{
	}

	void SpasideRemoteDevice::WatchdogTimeoutOccurred()
	{
	}

	void SpasideRemoteDevice::Slot_Spaside_Status(const Messages::JandyMessage_Status& msg)
	{
		// The master addressed us (the LED-image poll). Arm the one-shot window: the next Ack to
		// the master is our reply. (Phase 3 will decode the LED bytes carried by this frame.)
		m_AwaitingButtonAck = true;
		++m_PollCount;
	}

	void SpasideRemoteDevice::Slot_Spaside_Ack(const Messages::JandyMessage_Ack& msg)
	{
		if (!m_AwaitingButtonAck)
		{
			// Not the Ack that immediately followed the master polling us.
			return;
		}

		// Consume the one-shot window: a present remote answers every poll, so the first Ack
		// after our poll is ours. Clearing here also bounds any mis-attribution if we ever go
		// silent (the very next unrelated Ack would be ignored, not chained).
		m_AwaitingButtonAck = false;

		// Spa-side button reports carry ack_type 0x00 ([0x01][0x00][button]); OneTouch acks use
		// 0x80 and PDA 0x40, so this distinguishes our reply from those other devices' acks.
		if (static_cast<uint8_t>(msg.AckType()) != 0x00)
		{
			return;
		}

		const uint8_t button = msg.Command();
		if (button != 0x00)
		{
			m_LastButton = button;
			m_LastButtonAt = std::chrono::system_clock::now();
			LogInfo(Channel::Devices, [this, button]() { return std::format("SpasideRemote (0x{:02x}): button {} pressed", DeviceId().Id()(), button); });
		}
	}

	nlohmann::json SpasideRemoteDevice::DescribeDiagnostics() const
	{
		nlohmann::json j;

		j["device_type"] = "SpasideRemote";
		j["device_id"] = std::format("0x{:02x}", DeviceId().Id()());
		j["poll_count"] = static_cast<int64_t>(m_PollCount);
		j["last_button"] = static_cast<int>(m_LastButton);

		if (m_LastButtonAt.has_value())
		{
			const auto age = std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now() - m_LastButtonAt.value()).count();
			j["last_button_age_seconds"] = static_cast<int64_t>(age);
		}
		else
		{
			j["last_button_age_seconds"] = nullptr;
		}

		return j;
	}

}
// namespace AqualinkAutomate::Devices
