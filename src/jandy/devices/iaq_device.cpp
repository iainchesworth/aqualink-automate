#include <format>

#include "logging/logging.h"
#include "jandy/devices/iaq_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	IAQDevice::IAQDevice(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		IDevice(io_context, id, IAQ_TIMEOUT_DURATION),
		m_StatusPage(),
		m_TableInfo(),
		m_SM_PageUpdate(m_StatusPage),
		m_SM_TableUpdate(m_TableInfo)
	{
		m_SM_PageUpdate.initiate();
		m_SM_TableUpdate.initiate();

		Messages::IAQMessage_ControlReady::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_ControlReady, this, boost::placeholders::_1));
		Messages::IAQMessage_MessageLong::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_MessageLong, this, boost::placeholders::_1));
		Messages::IAQMessage_PageButton::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_PageButton, this, boost::placeholders::_1));
		Messages::IAQMessage_PageContinue::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_PageContinue, this, boost::placeholders::_1));
		Messages::IAQMessage_PageEnd::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_PageEnd, this, boost::placeholders::_1));
		Messages::IAQMessage_PageMessage::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_PageMessage, this, boost::placeholders::_1));
		Messages::IAQMessage_PageStart::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_PageStart, this, boost::placeholders::_1));
		Messages::IAQMessage_Poll::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_Poll, this, boost::placeholders::_1));
		Messages::IAQMessage_StartUp::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_StartUp, this, boost::placeholders::_1));
		Messages::IAQMessage_TableMessage::GetSignal()->connect(boost::bind(&IAQDevice::Slot_IAQ_TableMessage, this, boost::placeholders::_1));
	}

	IAQDevice::~IAQDevice()
	{
	}

	void IAQDevice::Slot_IAQ_ControlReady(const Messages::IAQMessage_ControlReady& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_ControlReady signal.");
	}

	void IAQDevice::Slot_IAQ_MessageLong(const Messages::IAQMessage_MessageLong& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_MessageLong signal.");
	}

	void IAQDevice::Slot_IAQ_PageButton(const Messages::IAQMessage_PageButton& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageButton signal.");
	}

	void IAQDevice::Slot_IAQ_PageContinue(const Messages::IAQMessage_PageContinue& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageContinue signal.");
	}

	void IAQDevice::Slot_IAQ_PageEnd(const Messages::IAQMessage_PageEnd& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageEnd signal.");
		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evEnd());
	}

	void IAQDevice::Slot_IAQ_PageMessage(const Messages::IAQMessage_PageMessage& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageMessage signal.");

		if (false)
		{
			LogDebug(Channel::Devices, "Received a PageMessage update out-of-band of a PageStart/PageEnd sequence.");
		}
		else if (IAQ_STATUS_PAGE_LINES <= msg.LineId())
		{
			LogDebug(Channel::Devices, std::format("Received a PageMessage update for an unsupported line; line id -> {}, content -> '{}'", msg.LineId(), msg.Line()));
		}
		else
		{
			m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evUpdating(msg.LineId(), msg.Line()));
		}
	}

	void IAQDevice::Slot_IAQ_PageStart(const Messages::IAQMessage_PageStart& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageStart signal.");
		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evStart());
	}

	void IAQDevice::Slot_IAQ_Poll(const Messages::IAQMessage_Poll& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_Poll signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void IAQDevice::Slot_IAQ_StartUp(const Messages::IAQMessage_StartUp& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_StartUp signal.");
	}

	void IAQDevice::Slot_IAQ_TableMessage(const Messages::IAQMessage_TableMessage& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_TableMessage signal.");

		if (IAQ_MESSAGE_TABLE_LINES <= msg.LineId())
		{
			LogDebug(Channel::Devices, std::format("Received a PageMessage update for an unsupported line; line id -> {}, content -> '{}'", msg.LineId(), msg.Line()));
		}
		else
		{
			m_TableInfo[msg.LineId()] = msg.Line();
		}
	}

}
// namespace AqualinkAutomate::Devices
