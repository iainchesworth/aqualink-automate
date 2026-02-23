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
		~AqualinkTouch_DeviceStatus() override;

	public:
		std::string ToString() const override;

	public:
		bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		bool DeserializeContents(std::span<const uint8_t> message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
