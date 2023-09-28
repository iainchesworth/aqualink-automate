#pragma once

#include <string>

#include <boost/beast/core/flat_buffer.hpp>

#include "http/webroute_equipment.h"
#include "http/websocket_equipment.h"

#include "support/unit_test_onetouchdevice.h"

namespace AqualinkAutomate::Test
{

	class Test_OneTouchDevicePlusHttpServer : public Test::OneTouchDevice
	{
	public:
		Test_OneTouchDevicePlusHttpServer();
		virtual ~Test_OneTouchDevicePlusHttpServer();

	public:
		void ReadFromHttpApi_NonBlocking(const std::string& api_route, boost::beast::flat_buffer& buffer);
		void ReadFromWebSocket_NonBlocking(boost::beast::flat_buffer& buffer);

	private:
		HTTP::WebRoute_Equipment m_API_Equipment;
		HTTP::WebSocket_Equipment m_WS_Equipment;
	};

}
// namespace AqualinkAutomate::Test
