#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>

#include "support/unit_test_onetouchdevice_httpserver.h"

namespace AqualinkAutomate::Test
{

	Test_OneTouchDevicePlusHttpServer::Test_OneTouchDevicePlusHttpServer() :
		Test::OneTouchDevice(),
		m_HTTPServer(1),
		m_API_Equipment(m_HTTPServer, DataHub(), StatisticsHub()),
		m_WS_Equipment(m_HTTPServer, DataHub())
	{
		m_HTTPServer.listen(LISTEN_ADDR, LISTEN_PORT);
	}

	Test_OneTouchDevicePlusHttpServer::~Test_OneTouchDevicePlusHttpServer()
	{
		boost::beast::error_code ec;

		http_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

		if (ws_thread.joinable())
		{
			m_HTTPServer.get_io_service().stop();
			ws_thread.join();
		}
	}

	void Test_OneTouchDevicePlusHttpServer::StartHttpServer()
	{
		m_HTTPServer.enable_timeout(false);			// Disable keep-alives.
		m_HTTPServer.enable_response_time(true);
	}

	void Test_OneTouchDevicePlusHttpServer::StartHttpClient()
	{
		auto const results = resolver.resolve(LISTEN_ADDR, LISTEN_PORT);

		http_stream.connect(results);

		m_HTTPServer.get_io_service().poll_one();
	}

	void Test_OneTouchDevicePlusHttpServer::StartWebSocketClient(const std::string& ws_route)
	{
		ws_thread = std::thread(
			[this]() -> void
			{
				m_HTTPServer.run();
			}
		);

		auto const results = resolver.resolve(LISTEN_ADDR, LISTEN_PORT);
		boost::asio::connect(ws_stream.next_layer(), results.begin(), results.end());

		ws_stream.set_option(boost::beast::websocket::stream_base::decorator(
			[](boost::beast::websocket::request_type& req)
			{
				req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " aqualink-automate-websocket-client");
			}));

		ws_stream.handshake(LISTEN_ADDR, ws_route);
	}

	void Test_OneTouchDevicePlusHttpServer::ReadFromHttpApi_Blocking(const std::string& api_route, boost::beast::flat_buffer& buffer, boost::beast::http::response<boost::beast::http::dynamic_body>& res)
	{
		boost::beast::http::request<boost::beast::http::string_body> req;

		req.method(boost::beast::http::verb::get);
		req.target(api_route);
		req.version(11);
		req.set(boost::beast::http::field::host, LISTEN_ADDR);
		req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " aqualink-automate-api-client");				

		boost::beast::http::write(http_stream, req);

		std::future<void> result_future = std::async(std::launch::async,
			[this]() -> void
			{
				m_HTTPServer.get_io_service().run_one();
				m_HTTPServer.get_io_service().run_one();
			}
		);

		boost::beast::http::read(http_stream, buffer, res);

		result_future.get();
	}

	void Test_OneTouchDevicePlusHttpServer::ReadFromWebSocket_Blocking(boost::beast::flat_buffer& buffer)
	{
		ws_stream.read(buffer);
	}

}
// namespace AqualinkAutomate::Test
