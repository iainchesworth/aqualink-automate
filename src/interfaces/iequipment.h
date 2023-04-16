#pragma once

#include <memory.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/bind/bind.hpp>
#include <boost/signals2.hpp>

namespace AqualinkAutomate::Interfaces
{
    template<typename PROTOCOL_HANDLER, typename MESSAGE_BRIDGE>
	class IEquipment
	{
	public:
		typedef PROTOCOL_HANDLER ProtocolHandler;
		typedef MESSAGE_BRIDGE MessageBridge;

	public:
		IEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler, MessageBridge& message_bridge) :
			m_ProtocolHandler(protocol_handler),
			m_MessageBridge(message_bridge)
		{
		}

	public:
		boost::asio::awaitable<void> Run()
		{
			co_return;
		}

	protected:
		ProtocolHandler& m_ProtocolHandler;
		MessageBridge& m_MessageBridge;
	};
}
// namespace AqualinkAutomate::Interfaces
