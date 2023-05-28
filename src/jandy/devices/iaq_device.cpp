#include <format>
#include <functional>

#include "logging/logging.h"
#include "jandy/devices/iaq_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	IAQDevice::IAQDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Config::JandyConfig& config, JandyControllerOperatingModes op_mode) :
		JandyController(io_context, device_id, IAQ_TIMEOUT_DURATION, config, op_mode),
		m_StatusPage(IAQ_STATUS_PAGE_LINES),
		m_TableInfo(IAQ_MESSAGE_TABLE_LINES),
		m_SM_PageUpdate(m_StatusPage),
		m_SM_TableUpdate(m_TableInfo)
	{
		m_SM_PageUpdate.initiate();
		m_SM_TableUpdate.initiate();

		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_ControlReady>(std::bind(&IAQDevice::Slot_IAQ_ControlReady, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_MessageLong>(std::bind(&IAQDevice::Slot_IAQ_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageButton>(std::bind(&IAQDevice::Slot_IAQ_PageButton, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageContinue>(std::bind(&IAQDevice::Slot_IAQ_PageContinue, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageEnd>(std::bind(&IAQDevice::Slot_IAQ_PageEnd, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageMessage>(std::bind(&IAQDevice::Slot_IAQ_PageMessage, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageStart>(std::bind(&IAQDevice::Slot_IAQ_PageStart, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_Poll>(std::bind(&IAQDevice::Slot_IAQ_Poll, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_StartUp>(std::bind(&IAQDevice::Slot_IAQ_StartUp, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_TableMessage>(std::bind(&IAQDevice::Slot_IAQ_TableMessage, this, std::placeholders::_1), device_id());
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
		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evSequenceEnd());
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
			m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evUpdate(msg.LineId(), msg.Line()));
		}
	}

	void IAQDevice::Slot_IAQ_PageStart(const Messages::IAQMessage_PageStart& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageStart signal.");
		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evSequenceStart());
		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evClear());
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
			m_TableInfo[msg.LineId()].Text = msg.Line();
		}
	}

}
// namespace AqualinkAutomate::Devices
