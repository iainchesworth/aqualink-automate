#include <cmath>
#include <functional>
#include <ranges>

#include <boost/bind/bind.hpp>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/utility/string_manipulation.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	OneTouchDevice::OneTouchDevice(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		IDevice(io_context, id, ONETOUCH_TIMEOUT_DURATION),
		m_DisplayedPage(ONETOUCH_PAGE_LINES),
		m_DisplayedPageUpdater(m_DisplayedPage),
		m_DisplayedPageProcessors
		{
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_OneTouch, { 11, "SYSTEM" }, std::bind(&OneTouchDevice::PageProcessor_OneTouch, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_System, { 0, "Jandy AquaLinkRS" }, std::bind(&OneTouchDevice::PageProcessor_System, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentStatus, { 0, "EQUIPMENT STATUS" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentStatus, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SelectSpeed, { 0, "Select Speed" }, std::bind(&OneTouchDevice::PageProcessor_SelectSpeed, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_MenuHelp, { 0, "Menu" }, std::bind(&OneTouchDevice::PageProcessor_MenuHelp, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTemperature, { 0, "Set TempS" }, std::bind(&OneTouchDevice::PageProcessor_SetTemperature, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTime, { 0, "Set Time" }, std::bind(&OneTouchDevice::PageProcessor_SetTime, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SystemSetup, { 0, "System Setup" }, std::bind(&OneTouchDevice::PageProcessor_SystemSetup, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_FreezeProtect, { 0, "Freeze Protect" }, std::bind(&OneTouchDevice::PageProcessor_FreezeProtect, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Boost, { 0, "Boost Pool" }, std::bind(&OneTouchDevice::PageProcessor_Boost, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetAquapure, { 0, "Set AQUAPURE" }, std::bind(&OneTouchDevice::PageProcessor_SetAquapure, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Version, { 7, "REV " }, std::bind(&OneTouchDevice::PageProcessor_Version, this, std::placeholders::_1))
		}
	{
		m_DisplayedPageUpdater.initiate();

		Messages::JandyMessage_MessageLong::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_MessageLong, this, boost::placeholders::_1));
		Messages::JandyMessage_Probe::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_Probe, this, boost::placeholders::_1));
		Messages::JandyMessage_Status::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_Status, this, boost::placeholders::_1));
		Messages::PDAMessage_Clear::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_Clear, this, boost::placeholders::_1));
		Messages::PDAMessage_Highlight::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_Highlight, this, boost::placeholders::_1));
		Messages::PDAMessage_HighlightChars::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_HighlightChars, this, boost::placeholders::_1));
		Messages::PDAMessage_ShiftLines::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_ShiftLines, this, boost::placeholders::_1));
	}

	OneTouchDevice::~OneTouchDevice()
	{
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
			m_DisplayedPageUpdater.process_event(Utility::ScreenDataPageUpdaterImpl::evUpdate(msg.LineId(), msg.Line()));
		}

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Probe signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a JandyMessage_Status signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Clear signal.");

		m_DisplayedPageUpdater.process_event(Utility::ScreenDataPageUpdaterImpl::evClear());

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Highlight signal.");

		// Process the "page" to extract information.
		auto actionable_processors = m_DisplayedPageProcessors | std::views::filter([this](const decltype(m_DisplayedPageProcessors)::value_type& processor) { return processor.CanProcess(m_DisplayedPage); });
		for (auto& processor : actionable_processors)
		{
			processor.Process(m_DisplayedPage);
		}

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_HighlightChars signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_ShiftLines signal.");

		auto direction = (0 > msg.LineShift()) ? Utility::ScreenDataPage::ShiftDirections::Up : Utility::ScreenDataPage::ShiftDirections::Down;
		auto lines_to_shift = std::abs(msg.LineShift());

		m_DisplayedPageUpdater.process_event(Utility::ScreenDataPageUpdaterImpl::evShift(direction, msg.FirstLineId(), msg.LastLineId(), lines_to_shift));

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::PageProcessor_OneTouch(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_System(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 0 = Equipment Status		Equipment Status	Equipment Status
			Info:   OneTouch Menu Line 1 = 
			Info:   OneTouch Menu Line 2 = Intelliflo VS 3		Intelliflo VS 3		Intelliflo VF 2 
			Info:   OneTouch Menu Line 3 =  *** Priming ***		     RPM: 2750		     RPM: 2250
			Info:   OneTouch Menu Line 4 =     Watts: 100             RPM: 600		    Watts: 55
			Info:   OneTouch Menu Line 5 =                          Watts: 55		      GPM: 80
			Info:   OneTouch Menu Line 6 = 
			Info:   OneTouch Menu Line 7 = 
			Info:   OneTouch Menu Line 8 = 
			Info:   OneTouch Menu Line 9 = 
			Info:   OneTouch Menu Line 10 = 
			Info:   OneTouch Menu Line 11 = 
		*/
	}

	void OneTouchDevice::PageProcessor_SelectSpeed(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_MenuHelp(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_SetTemperature(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 0 =     Set Temp
			Info:   OneTouch Menu Line 1 =
			Info:   OneTouch Menu Line 2 = Pool Heat   90`F
			Info:   OneTouch Menu Line 3 = Spa Heat   102`F
			Info:   OneTouch Menu Line 4 =
			Info:   OneTouch Menu Line 5 = Maintain     OFF
			Info:   OneTouch Menu Line 6 = Hours  12AM-12AM
			Info:   OneTouch Menu Line 7 =
			Info:   OneTouch Menu Line 8 = Highlight an
			Info:   OneTouch Menu Line 9 = item and press
			Info:   OneTouch Menu Line 10 = Select
			Info:   OneTouch Menu Line 11 =
		*/
	}

	void OneTouchDevice::PageProcessor_SetTime(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_SystemSetup(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 0 =  Freeze Protect
			Info:   OneTouch Menu Line 1 =
			Info:   OneTouch Menu Line 2 =
			Info:   OneTouch Menu Line 3 = Temp        38`F
			Info:   OneTouch Menu Line 4 =
			Info:   OneTouch Menu Line 5 =
			Info:   OneTouch Menu Line 6 = Use Arrow Keys
			Info:   OneTouch Menu Line 7 = to set value.
			Info:   OneTouch Menu Line 8 = Press SELECT
			Info:   OneTouch Menu Line 9 = to continue.
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =
		*/
	}

	void OneTouchDevice::PageProcessor_Boost(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_SetAquapure(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_Version(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 0 =
			Info:   OneTouch Menu Line 1 =
			Info:   OneTouch Menu Line 2 =
			Info:   OneTouch Menu Line 3 =
			Info:   OneTouch Menu Line 4 =     B0029221
			Info:   OneTouch Menu Line 5 =    RS-8 Combo
			Info:   OneTouch Menu Line 6 =
			Info:   OneTouch Menu Line 7 =    REV T.0.1
			Info:   OneTouch Menu Line 8 =
			Info:   OneTouch Menu Line 9 =
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =
		*/

		auto serial_number = Utility::TrimWhitespace(page[4]);
		auto panel_type = Utility::TrimWhitespace(page[5]);
		auto fw_revision = Utility::TrimWhitespace(page[7]);
	}

}
// namespace AqualinkAutomate::Devices
