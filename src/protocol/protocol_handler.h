#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>

#include <boost/circular_buffer.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "messages/message.h"
#include "messages/message_statemachine.h"
#include "serial/serial_port.h"

namespace AqualinkAutomate::Protocol
{

	class ProtocolHandler
	{
		static constexpr uint32_t READ_BUFFER_LEN = 1024;

	public:
		ProtocolHandler(std::shared_ptr<AqualinkAutomate::Serial::SerialPort> serial_port);

	public:
		boost::asio::awaitable<void> HandleProtocol();

	private:
		virtual std::expected<std::shared_ptr< AqualinkAutomate::Messages::Message>, uint32_t> GenerateMessageFromRawData() = 0;

	private:
		virtual void HandleMessage(const AqualinkAutomate::Messages::Message& message) = 0;
		virtual void SendResponse(const AqualinkAutomate::Messages::Message& message) = 0;

	protected:
		Messages::MessageGeneratorMachine m_MessageStateMachine;
		boost::circular_buffer<uint8_t> m_MessageDataBuffer;
		std::mutex m_MessageDataBufferMutex;

	private:
		std::shared_ptr<AqualinkAutomate::Serial::SerialPort> m_SerialPort;
	};

}
// namespace AqualinkAutomate::Protocol
