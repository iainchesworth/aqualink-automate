#pragma once

#include <boost/asio/awaitable.hpp>

#include "serial/serial_port.h"

using namespace AqualinkAutomate::Serial;

namespace AqualinkAutomate::Equipment
{
	template<typename MESSAGE_HANDLER, typename PROTOCOL_HANDLER>
	class EquipmentBase
	{
	public:
		EquipmentBase(Serial::SerialPort& serial_port) :
			m_MessageHandler(),
			m_ProtocolHandler(serial_port)
		{
		}

	public:
		boost::asio::awaitable<void> Run()
		{
			bool continue_processing = true;

			do
			{
				if (auto result = co_await m_ProtocolHandler.HandleProtocol(); !result.has_value())
				{
					continue_processing = false;
				}
				else
				{
					///TODO - process the message....
				}

			} while (continue_processing);
		}

	private:
		MESSAGE_HANDLER m_MessageHandler;
		PROTOCOL_HANDLER m_ProtocolHandler;
	};
}
// namespace AqualinkAutomate::Equipment
