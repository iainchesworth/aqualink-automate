#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "jandy/messages/jandy_message_probe.h"

namespace AqualinkAutomate::Devices
{

	class OneTouchDevice_Emulated : public Interfaces::IDevice
	{
		const std::chrono::seconds ONETOUCH_TIMEOUT_DURATION = std::chrono::seconds(30);

	public:
		OneTouchDevice_Emulated(boost::asio::io_context& io_context, IDevice::DeviceId id);
		virtual ~OneTouchDevice_Emulated();

	private:
		void Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg);
	};

}
// namespace AqualinkAutomate::Devices
