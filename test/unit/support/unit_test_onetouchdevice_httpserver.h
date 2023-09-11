#pragma once

#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <thread>

#ifdef windowBits
#undef windowBits // Fix a macro issue in cinatra which impacts boost::beast.
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "http/webroute_types.h"
#include "http/webroute_equipment.h"
#include "http/websocket_equipment.h"

#include "support/unit_test_onetouchdevice.h"

namespace AqualinkAutomate::Test
{

	class Test_OneTouchDevicePlusHttpServer : public Test::OneTouchDevice
	{
		static const std::string LISTEN_ADDR;
		static const std::string LISTEN_PORT;

	public:
		Test_OneTouchDevicePlusHttpServer();
		virtual ~Test_OneTouchDevicePlusHttpServer();

	public:
		void StartHttpServer();
		void StartHttpClient();
		void StartWebSocketClient(const std::string& ws_route);

	public:
		void ReadFromHttpApi_NonBlocking(const std::string& api_route, boost::beast::flat_buffer& buffer, boost::beast::http::response<boost::beast::http::dynamic_body>& res);
		void ReadFromWebSocket_NonBlocking(boost::beast::flat_buffer& buffer);
		void BlockForAsyncOperationToComplete();

	private:
		static const std::string GenerateListeningPort();

	private:
		HTTP::Server m_HTTPServer;
		HTTP::WebRoute_Equipment m_API_Equipment;
		HTTP::WebSocket_Equipment m_WS_Equipment;
		std::thread m_HTTPServerThread;

	private:
		boost::asio::io_context m_IOContext;
		boost::asio::ip::tcp::resolver resolver{ m_IOContext };
		boost::beast::tcp_stream http_stream{ m_IOContext };
		boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_stream{ m_IOContext };

	private:
		boost::asio::chrono::seconds m_ReadTimeout{ 5 };
		boost::asio::steady_timer m_ReadTimer{ m_IOContext, m_ReadTimeout };
	};

}
// namespace AqualinkAutomate::Test
