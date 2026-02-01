#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/serial_adapter/serial_adapter_message.h"

namespace AqualinkAutomate::Messages
{

	class SerialAdapterMessage_DevReady : public SerialAdapterMessage, public Interfaces::IMessageSignalRecv<SerialAdapterMessage_DevReady>
	{
	public:
		SerialAdapterMessage_DevReady() noexcept;
		virtual ~SerialAdapterMessage_DevReady();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
