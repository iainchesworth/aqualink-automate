#include <cmath>
#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void OneTouchDevice::Slot_OneTouch_MessageLong(const Messages::JandyMessage_MessageLong& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_MessageLong signal.");

		if (ONETOUCH_PAGE_LINES <= msg.LineId())
		{
			LogDebug(Channel::Devices, std::format("OneTouch device received a MessageLong update for an unsupported line; line id -> {}, content -> '{}'", msg.LineId(), msg.Line()));
		}
		else
		{
			m_OpState = OperatingStates::ScreenUpdating;
			m_DisplayedPageUpdater.process_event(Utility::ScreenDataPageUpdaterImpl::evUpdate(msg.LineId(), msg.Line()));
			Signal_OneTouch_AckMessage();
			HandleAnyInternalProcessing();
		}

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Probe signal.");

		Signal_OneTouch_AckMessage();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Status signal.");

		Signal_OneTouch_AckMessage();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Clear signal.");

		m_DisplayedPageUpdater.process_event(Utility::ScreenDataPageUpdaterImpl::evClear());
		Signal_OneTouch_AckMessage();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Highlight signal.");

		Signal_OneTouch_AckMessage();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_HighlightChars signal.");

		Signal_OneTouch_AckMessage();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_ShiftLines signal.");

		auto direction = (0 > msg.LineShift()) ? Utility::ScreenDataPage::ShiftDirections::Up : Utility::ScreenDataPage::ShiftDirections::Down;
		auto lines_to_shift = std::abs(msg.LineShift());

		m_OpState = OperatingStates::ScreenUpdating;
		m_DisplayedPageUpdater.process_event(Utility::ScreenDataPageUpdaterImpl::evShift(direction, msg.FirstLineId(), msg.LastLineId(), lines_to_shift));
		Signal_OneTouch_AckMessage();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Unknown(const Messages::JandyMessage_Unknown& msg)
	{
		LogDebug(Channel::Devices, std::format("OneTouch device received a JandyMessage_Unknown signal: type -> 0x{:02x}", static_cast<uint8_t>(msg.Id())));

		Signal_OneTouch_AckMessage();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Signal_OneTouch_AckMessage() const
	{
		if (JandyDeviceOperatingModes::Emulated == m_OpMode)
		{
			LogDebug(Channel::Devices, "Emulated OneTouch device sending ACKnowledgement message");

			auto ack_message = std::make_shared<Messages::JandyMessage_Ack>(Messages::AckTypes::V1_Normal, 0x00);
			ack_message->Signal_MessageToSend();
		}
	}

}
// namespace AqualinkAutomate::Devices
