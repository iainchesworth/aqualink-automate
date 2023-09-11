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
	const std::string Test_OneTouchDevicePlusHttpServer::LISTEN_ADDR{ "127.0.0.1" };
	const std::string Test_OneTouchDevicePlusHttpServer::LISTEN_PORT{ Test_OneTouchDevicePlusHttpServer::GenerateListeningPort() };

	Test_OneTouchDevicePlusHttpServer::Test_OneTouchDevicePlusHttpServer() :
		Test::OneTouchDevice(),
		m_HTTPServer(1),
		m_API_Equipment(m_HTTPServer, *this),
		m_WS_Equipment(m_HTTPServer, *this),
		m_HTTPServerThread()
	{
		BOOST_TEST_REQUIRE(m_HTTPServer.listen(LISTEN_ADDR, LISTEN_PORT));
		
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

		m_IOContext.stop();
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
		boost::asio::ip::tcp::resolver::results_type results;
	
		BOOST_REQUIRE_NO_THROW(results = resolver.resolve(LISTEN_ADDR, LISTEN_PORT));
		BOOST_TEST_REQUIRE(1 == results.size());
		BOOST_REQUIRE_NO_THROW(http_stream.connect(results));
	}

	void Test_OneTouchDevicePlusHttpServer::StartWebSocketClient(const std::string& ws_route)
	{
		boost::asio::ip::tcp::resolver::results_type results;

		BOOST_REQUIRE_NO_THROW(results = resolver.resolve(LISTEN_ADDR, LISTEN_PORT));
		BOOST_TEST_REQUIRE(1 == results.size());
		BOOST_REQUIRE_NO_THROW(boost::asio::connect(ws_stream.next_layer(), results.begin(), results.end()));

		ws_stream.set_option(boost::beast::websocket::stream_base::decorator(
			[](boost::beast::websocket::request_type& req)
			{
				req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " aqualink-automate-websocket-client");
			}));

		ws_stream.handshake(LISTEN_ADDR, ws_route);
	}

	void Test_OneTouchDevicePlusHttpServer::ReadFromHttpApi_NonBlocking(const std::string& api_route, boost::beast::flat_buffer& buffer, boost::beast::http::response<boost::beast::http::dynamic_body>& res)
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
							BOOST_TEST_REQUIRE(0 < m_ReadTimer.cancel());
							m_IOContext.stop();
							m_IOContext.restart();
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
					BOOST_ERROR("Timeout while waiting for socket response from HTTP server during test");
					m_IOContext.stop();
				}
			}
		);

		BlockForAsyncOperationToComplete();
	}

	void Test_OneTouchDevicePlusHttpServer::ReadFromWebSocket_NonBlocking(boost::beast::flat_buffer& buffer)
	{
		m_ReadTimer.async_wait(
			[this](const boost::system::error_code& ec) -> void
			{
				if (boost::asio::error::operation_aborted == ec.value())
				{
					// Everything is fine -> the timer was cancelled so it's all good.
				}
				else
				{
					BOOST_ERROR("Timeout while waiting for websocket response from HTTP server during test");
					m_IOContext.stop();
				}
			}
		);

		ws_stream.async_read(buffer, 
			[this](const boost::system::error_code& ec, std::size_t bytes_transferred) -> void
			{
				BOOST_TEST_REQUIRE(0 < m_ReadTimer.cancel());

				if (boost::system::errc::success == ec.value())
				{
					// Everything is fine -> the read completed so it's all good.
				}
				else
				{
					BOOST_ERROR("Error while processing async_read from websocket stream during test");
					m_IOContext.stop();
				}
			}
		);
	}

	void Test_OneTouchDevicePlusHttpServer::BlockForAsyncOperationToComplete()
	{
		if (m_IOContext.stopped())
		{
			m_IOContext.restart();
		}
		
		BOOST_TEST_REQUIRE(!m_IOContext.stopped());

		m_IOContext.run();
	}

	const std::string Test_OneTouchDevicePlusHttpServer::GenerateListeningPort()
	{
		// It is entirely possible that a fixed ephemeral port is locked and cannot be used
		// and so attempt to reduce the probability by picking a random from the appropriate
		// range.

		std::random_device random_device;
		std::mt19937 generator(random_device());
		std::uniform_int_distribution<int> distribution(49152, 65535);
		uint16_t random_number = distribution(generator);
		return std::to_string(random_number);
	}

}
// namespace AqualinkAutomate::Test
