#include <cmath>
#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/messages/jandy_message_ack.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void OneTouchDevice::Slot_OneTouch_Ack(const Messages::JandyMessage_Ack& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Ack signal.");

		KeyCommands key_press = msg.Command<KeyCommands>([](uint8_t command_id)
			{
				return magic_enum::enum_cast<KeyCommands>(command_id).value_or(KeyCommands::Unknown);
			}
		);

		LogDebug(Channel::Devices, std::format("Decoded OneTouch-specific JandyMessage_Ack payload: key press -> {} (raw: 0x{:02x})", magic_enum::enum_name(key_press), msg.Command()));

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

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

		if (OperatingStates::ScreenUpdating == m_OpState)
		{
			// The series of JandyMessage_MessageLong messages has finished.
			m_OpState = OperatingStates::ScreenUpdateComplete;
		}

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
		if (JandyControllerOperatingModes::Emulated == m_OpMode)
		{
			LogDebug(Channel::Devices, "Emulated OneTouch device sending ACKnowledgement message");

			auto ack_message = std::make_shared<Messages::JandyMessage_Ack>(Messages::AckTypes::V2_Normal, 0x00);
			ack_message->Signal_MessageToSend();
		}
	}

}
// namespace AqualinkAutomate::Devices
