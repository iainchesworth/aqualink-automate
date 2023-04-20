#include "jandy/devices/iaq_device.h"

namespace AqualinkAutomate::Devices
{

	IAQDevice::IAQDevice(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		IDevice(io_context, id, IAQ_TIMEOUT_DURATION)
	{
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
	}

	void IAQDevice::Slot_IAQ_PageMessage(const Messages::IAQMessage_PageMessage& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageMessage signal.");
	}

	void IAQDevice::Slot_IAQ_PageStart(const Messages::IAQMessage_PageStart& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_PageStart signal.");
	}

	void IAQDevice::Slot_IAQ_Poll(const Messages::IAQMessage_Poll& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_Poll signal.");
	}

	void IAQDevice::Slot_IAQ_StartUp(const Messages::IAQMessage_StartUp& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_StartUp signal.");
	}

	void IAQDevice::Slot_IAQ_TableMessage(const Messages::IAQMessage_TableMessage& msg)
	{
		LogDebug(Channel::Devices, "IAQ device received a IAQMessage_TableMessage signal.");
	}

}
// namespace AqualinkAutomate::Devices
