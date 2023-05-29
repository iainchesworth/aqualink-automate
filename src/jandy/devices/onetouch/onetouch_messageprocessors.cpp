#include <cmath>
#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/formatters/screen_data_page_formatter.h"
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
			JandyController::m_Screen.ScreenMode(JandyScreenModes::Updating);
			JandyController::m_Screen.ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evUpdate(msg.LineId(), msg.Line()));
			JandyController::m_Screen.ProcessScreenUpdates();

			Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
			HandleAnyInternalProcessing();
		}

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Probe signal.");

		Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Status signal.");

		if (JandyScreenModes::Updating == JandyController::m_Screen.ScreenMode())
		{
			LogInfo(Channel::Devices, std::format("\n{}", JandyController::m_Screen.DisplayedPage()));

			// The series of JandyMessage_MessageLong messages has finished.
			JandyController::m_Screen.ScreenMode(JandyScreenModes::UpdateComplete);
		}

		if ((OperatingStates::NormalOperation == m_OpState) && (JandyScreenModes::Normal == JandyController::m_Screen.ScreenMode()) && (m_InitialisationRequired))
		{
			++m_InitialisationGraphIterator;

			if (Utility::ScreenDataPageGraphImpl::ForwardIterator::end(m_InitialisationGraph) == m_InitialisationGraphIterator)
			{
				m_InitialisationRequired = false;
			}
			else
			{
				auto [new_vertex_id, edge_traversed] = *m_InitialisationGraphIterator;
				if (edge_traversed.key_command.has_value())
				{
					try
					{
						auto key_command_to_send = std::any_cast<KeyCommands>(edge_traversed.key_command);

						// Send an ACK message with a keypress command; this is because we're traversing the menu.
						Signal_OneTouch_AckMessage(key_command_to_send);
					}
					catch (const std::bad_any_cast& eAC)
					{
						///FIXME LOGGING.
					}
				}
			}
		}
		else
		{
			// Just send a normal ACK message; don't issue a command.
			Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
		}

		JandyController::m_Screen.ProcessScreenUpdates();
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Clear signal.");

		JandyController::m_Screen.ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evClear());
		Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Highlight signal.");

		JandyController::m_Screen.ScreenMode(JandyScreenModes::Updating);
		JandyController::m_Screen.ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evHighlight(msg.LineId()));

		Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_HighlightChars signal.");

		JandyController::m_Screen.ScreenMode(JandyScreenModes::Updating);
		JandyController::m_Screen.ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evHighlightChars(msg.LineId(), msg.StartIndex(), msg.StopIndex()));

		Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_ShiftLines signal.");

		auto direction = (0 > msg.LineShift()) ? Utility::ScreenDataPage::ShiftDirections::Up : Utility::ScreenDataPage::ShiftDirections::Down;
		auto lines_to_shift = std::abs(msg.LineShift());

		JandyController::m_Screen.ScreenMode(JandyScreenModes::Updating);
		JandyController::m_Screen.ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evShift(direction, msg.FirstLineId(), msg.LastLineId(), lines_to_shift));
		JandyController::m_Screen.ProcessScreenUpdates();
		
		Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Unknown(const Messages::JandyMessage_Unknown& msg)
	{
		LogDebug(Channel::Devices, std::format("OneTouch device received a JandyMessage_Unknown signal: type -> 0x{:02x}", static_cast<uint8_t>(msg.Id())));

		Signal_OneTouch_AckMessage(KeyCommands::NoKeyCommand);
		HandleAnyInternalProcessing();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Signal_OneTouch_AckMessage(KeyCommands key_command_to_send) const
	{
		if (JandyControllerOperatingModes::Emulated == m_OpMode)
		{
			LogDebug(Channel::Devices, std::format("Emulated OneTouch device sending ACKnowledgement message (with command: {})", magic_enum::enum_name(key_command_to_send)));

			auto ack_message = std::make_shared<Messages::JandyMessage_Ack>(Messages::AckTypes::V2_Normal, static_cast<uint8_t>(magic_enum::enum_integer(key_command_to_send)));
			ack_message->Signal_MessageToSend();
		}
	}

}
// namespace AqualinkAutomate::Devices
