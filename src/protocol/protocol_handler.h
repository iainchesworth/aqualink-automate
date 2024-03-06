#pragma once

#include <cstdint>
#include <vector>

#include <boost/cobalt/promise.hpp>
#include <boost/signals2/connection.hpp>

#include "coroutines/awaitable_signal.h"
#include "logging/logging.h"
#include "serial/serial_port.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Protocol
{

	boost::cobalt::promise<void> ProtocolHandler_ReadOp(Serial::SerialPort& serial_port);

	boost::cobalt::promise<void> ProtocolHandler_WriteOp_MessagePublisher(Serial::SerialPort& serial_port, std::vector<uint8_t> buffer);

	template<typename MESSAGE_PUBLISHER>
	boost::cobalt::promise<void> ProtocolHandler_WriteOp(Serial::SerialPort& serial_port)
	{	
		using SignalPayload = typename MESSAGE_PUBLISHER::PublisherRef;

		if (auto signal_ptr = MESSAGE_PUBLISHER::GetPublisher(); nullptr == signal_ptr)
		{
			///FIXME
		}
		else
		{
			while (!co_await boost::cobalt::this_coro::cancelled)
			{
				SignalPayload signal_payload = co_await Coroutines::AwaitSignal<SignalPayload>(*signal_ptr);
				if (std::vector<uint8_t> buffer; !(signal_payload.get().Serialize(buffer)))
				{
					/// FIXME
				}
				else
				{
					co_await ProtocolHandler_WriteOp_MessagePublisher(serial_port, buffer);
				}
			}
		}
	}

}
// namespace AqualinkAutomate::Protocol
