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
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_Ack", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
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
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_MessageLong", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_MessageLong signal.");

		if (ONETOUCH_PAGE_LINES <= msg.LineId())
		{
			LogDebug(Channel::Devices, std::format("OneTouch device received a MessageLong update for an unsupported line; line id -> {}, content -> '{}'", msg.LineId(), msg.Line()));
		}
		else
		{
			ScreenMode(Capabilities::ScreenModes::Updating);
			ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evUpdate(msg.LineId(), msg.Line()));
			ProcessScreenUpdates();

			ProcessControllerUpdates();
		}

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_Probe", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Probe signal.");

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_Status", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Status signal.");

		if (Capabilities::ScreenModes::Updating == ScreenMode())
		{
			LogDebug(Channel::Devices, std::format("\n{}", DisplayedPage()));

			// The series of JandyMessage_MessageLong messages has finished.
			ScreenMode(Capabilities::ScreenModes::UpdateComplete);
		}

		ProcessScreenUpdates();
		ProcessControllerUpdates();

		// All start-up messages upto (and including) the first status message have a 
		// different ACKnowledgement type so now the first status message has been
		// ACKed, switch to the next type.
		if (Messages::AckTypes::V1_Normal == m_AckType_ToSend)
		{
			m_AckType_ToSend = Messages::AckTypes::V2_Normal;
		}

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_Clear", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Clear signal.");

		ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evClear());
		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_Highlight", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Highlight signal.");

		ScreenMode(Capabilities::ScreenModes::Updating);
		ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evHighlight(msg.LineId()));

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_HighlightChars", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_HighlightChars signal.");

		ScreenMode(Capabilities::ScreenModes::Updating);
		ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evHighlightChars(msg.LineId(), msg.StartIndex(), msg.StopIndex()));

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_ShiftLines", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_ShiftLines signal.");

		auto direction = (0 > msg.LineShift()) ? Utility::ScreenDataPage::ShiftDirections::Up : Utility::ScreenDataPage::ShiftDirections::Down;
		auto lines_to_shift = std::abs(msg.LineShift());

		ScreenMode(Capabilities::ScreenModes::Updating);
		ProcessScreenEvent(Utility::ScreenDataPageUpdaterImpl::evShift(direction, msg.FirstLineId(), msg.LastLineId(), lines_to_shift));
		ProcessScreenUpdates();
		
		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Unknown(const Messages::JandyMessage_Unknown& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouch MessageProcessors -> Slot_OneTouch_Unknown", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);
		LogDebug(Channel::Devices, std::format("OneTouch device received a JandyMessage_Unknown signal: type -> 0x{:02x}", static_cast<uint8_t>(msg.Id())));

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

}
// namespace AqualinkAutomate::Devices
