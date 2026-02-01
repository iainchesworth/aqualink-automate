#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/aqualink_touch/aqualink_touch_message.h"

namespace AqualinkAutomate::Messages
{

	class AqualinkTouch_DeviceStatus : public AqualinkTouchMessage, public Interfaces::IMessageSignalRecv<AqualinkTouch_DeviceStatus>
	{
	public:
		AqualinkTouch_DeviceStatus() noexcept;
		virtual ~AqualinkTouch_DeviceStatus();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
