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

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void OneTouchDevice::Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg)
	{
		LogDebug(Channel::Devices, "OneTouch device received a PDAMessage_Clear signal.");

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

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

}
// namespace AqualinkAutomate::Devices
