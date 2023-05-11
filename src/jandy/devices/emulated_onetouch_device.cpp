#include "jandy/devices/emulated_onetouch_device.h"
#include "jandy/messages/jandy_message_ack.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	OneTouchDevice_Emulated::OneTouchDevice_Emulated(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		IDevice(io_context, id, ONETOUCH_TIMEOUT_DURATION)
	{
		Messages::JandyMessage_Probe::GetSignal()->connect(boost::bind(&OneTouchDevice_Emulated::Slot_OneTouch_Probe, this, boost::placeholders::_1));
	}

	OneTouchDevice_Emulated::~OneTouchDevice_Emulated()
	{
	}

	void OneTouchDevice_Emulated::Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg)
	{
		LogTrace(Channel::Devices, "Emulated OneTouch device received a JandyMessage_Probe signal.");

		if (msg.Destination().Raw() == Id())
		{
			LogDebug(Channel::Devices, "Responding to an PROBE for this emulated OneTouch device id");

			auto ack_message = std::make_shared<Messages::JandyMessage_Ack>();
			ack_message->Signal();
		}

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

}
// namespace AqualinkAutomate::Devices
