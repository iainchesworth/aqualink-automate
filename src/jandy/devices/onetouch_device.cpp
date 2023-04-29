#include <cmath>

#include "jandy/devices/onetouch_device.h"

namespace AqualinkAutomate::Devices
{

	OneTouchDevice::OneTouchDevice(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		IDevice(io_context, id, ONETOUCH_TIMEOUT_DURATION),
		m_DisplayedPage(),
		m_DisplayedPageUpdater(m_DisplayedPage)
	{
		m_DisplayedPageUpdater.initiate();

		Messages::JandyMessage_MessageLong::GetSignal()->connect(boost::bind(&OneTouchDevice::Slot_OneTouch_MessageLong, this, boost::placeholders::_1));
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

		using Utility::ScreenDataPageUpdaterImpl::ShiftDirections;

		auto direction = (0 > msg.LineShift()) ? ShiftDirections::Up : ShiftDirections::Down;
		auto lines_to_shift = std::abs(msg.LineShift());

		m_DisplayedPageUpdater.process_event(Utility::ScreenDataPageUpdaterImpl::evShift(direction, msg.FirstLineId(), msg.LastLineId(), lines_to_shift));

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

}
// namespace AqualinkAutomate::Devices
