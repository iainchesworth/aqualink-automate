#include <boost/test/unit_test.hpp>

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
		m_API_Equipment(m_HTTPServer, *this),
		m_WS_Equipment(m_HTTPServer, *this),
		m_HTTPServerThread()
	{
		m_HTTPServer.listen(LISTEN_ADDR, LISTEN_PORT);
		
		m_HTTPServer.enable_timeout(false);			// Disable keep-alives.
		m_HTTPServer.enable_response_time(true);
	}

	Test_OneTouchDevicePlusHttpServer::~Test_OneTouchDevicePlusHttpServer()
	{
		boost::beast::error_code ec;

		http_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

		if (m_HTTPServerThread.joinable())
		{
			m_HTTPServer.get_io_service().stop();
			m_HTTPServerThread.join();
		}
	}

	void Test_OneTouchDevicePlusHttpServer::StartHttpServer()
	{
		m_HTTPServerThread = std::thread(
			[this]() -> void
			{
				m_HTTPServer.run();
			}
		);
	}

	void Test_OneTouchDevicePlusHttpServer::StartHttpClient()
	{
		auto const results = resolver.resolve(LISTEN_ADDR, LISTEN_PORT);
		http_stream.connect(results);
	}

	void Test_OneTouchDevicePlusHttpServer::StartWebSocketClient(const std::string& ws_route)
	{
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

		boost::beast::http::async_write(http_stream.socket(), req,
			[this, &buffer, &res](const boost::system::error_code& ec, std::size_t bytes_transferred) -> void
			{
				if (ec)
				{
					BOOST_ERROR("Error occurred while performing GET request for HTTP server method test");
				}
				else
				{
					boost::beast::http::async_read(http_stream, buffer, res,
						[this](const boost::system::error_code& ec, std::size_t bytes_transferred) -> void
						{
							m_ReadTimer.cancel_one();

							ioc.stop();
							ioc.reset();
						}
					);
				}
			}
		);

		m_ReadTimer.async_wait(
			[this](const boost::system::error_code& ec) ->void
			{
				if (boost::asio::error::operation_aborted == ec.value())
				{
					// Everything is fine, the timer was cancelled so it's all good.
				}
				else
				{
					ioc.stop();

					BOOST_ERROR("Timeout while waiting for response from HTTP server during test");
				}
			}
		);

		ioc.run();
	}

	void Test_OneTouchDevicePlusHttpServer::ReadFromWebSocket_Blocking(boost::beast::flat_buffer& buffer)
	{
		ws_stream.read(buffer);
	}

}
// namespace AqualinkAutomate::Test
