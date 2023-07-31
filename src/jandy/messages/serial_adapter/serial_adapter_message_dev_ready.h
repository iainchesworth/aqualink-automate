#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/serial_adapter/serial_adapter_message.h"

namespace AqualinkAutomate::Messages
{

	class SerialAdapterMessage_DevReady : public SerialAdapterMessage, public Interfaces::IMessageSignalRecv<SerialAdapterMessage_DevReady>
	{
	public:
		SerialAdapterMessage_DevReady();
		virtual ~SerialAdapterMessage_DevReady();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::SerialAdapterMessage_DevReady> g_SerialAdapterMessage_DevReady_Registration;
	};

}
// namespace AqualinkAutomate::Messages
