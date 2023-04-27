#include <format>

#include "logging/logging.h"
#include "jandy/devices/pda_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	PDADevice::PDADevice(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		IDevice(io_context, id, PDA_TIMEOUT_DURATION)
	{
		Messages::JandyMessage_Ack::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_Ack, this, boost::placeholders::_1));
		Messages::PDAMessage_Clear::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_Clear, this, boost::placeholders::_1));
		Messages::PDAMessage_Highlight::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_Highlight, this, boost::placeholders::_1));
		Messages::PDAMessage_HighlightChars::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_HighlightChars, this, boost::placeholders::_1));
		Messages::JandyMessage_MessageLong::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_MessageLong, this, boost::placeholders::_1));
		Messages::JandyMessage_Status::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_Status, this, boost::placeholders::_1));
		Messages::PDAMessage_ShiftLines::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_ShiftLines, this, boost::placeholders::_1));
		Messages::JandyMessage_Unknown::GetSignal()->connect(boost::bind(&PDADevice::Slot_PDA_Unknown_PDA_1B, this, boost::placeholders::_1));
	}

	PDADevice::~PDADevice()
	{
	}

	void PDADevice::Slot_PDA_Ack(const Messages::JandyMessage_Ack& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Ack signal.");
	}

	void PDADevice::Slot_PDA_Clear(const Messages::PDAMessage_Clear& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Clear signal.");
	}

	void PDADevice::Slot_PDA_Highlight(const Messages::PDAMessage_Highlight& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a PDAMessage_Highlight signal.");
	}

	void PDADevice::Slot_PDA_HighlightChars(const Messages::PDAMessage_HighlightChars& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a PDAMessage_HighlightChars signal.");
	}

	void PDADevice::Slot_PDA_MessageLong(const Messages::JandyMessage_MessageLong& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_MessageLong signal.");
	}

	void PDADevice::Slot_PDA_Status(const Messages::JandyMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Status signal.");
	}

	void PDADevice::Slot_PDA_ShiftLines(const Messages::PDAMessage_ShiftLines& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a PDAMessage_ShiftLines signal.");
	}

	void PDADevice::Slot_PDA_Unknown_PDA_1B(const Messages::JandyMessage_Unknown& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Unknown signal.");
	}

}
// namespace AqualinkAutomate::Devices
