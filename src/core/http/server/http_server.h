#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "http/server/server_types.h"
#include "interfaces/iwebsocket.h"

namespace AqualinkAutomate::HTTP
{

	/// Async HTTP session state machine.
	///
	/// Uses Beast async operations (async_read, async_write, etc.) that post
	/// completions to the io_context.  The main frame loop drives progress by
	/// calling io_context.poll().
	class HttpSessionState : public std::enable_shared_from_this<HttpSessionState>
	{
	public:
		explicit HttpSessionState(TcpSocket socket);
		explicit HttpSessionState(TcpSocket socket, boost::asio::ssl::context& ssl_ctx);

		void Start();
		void Poll();   // Kick WebSocket outbound writes
		bool IsDone() const;
		void Close();  // Force-close for shutdown

	private:
		void DoSslHandshake();
		void OnSslHandshake(boost::system::error_code ec);

		void DoRead();
		void OnRead(boost::system::error_code ec, std::size_t bytes_transferred);

		void ProcessRequest();

		void DoWrite(Message msg);
		void OnWrite(boost::system::error_code ec, std::size_t bytes_transferred, bool keep_alive);

		void DoWsAccept(const Request& req);
		void OnWsAccept(boost::system::error_code ec);

		void DoWsRead();
		void OnWsRead(boost::system::error_code ec, std::size_t bytes_transferred);

		void TryWsWrite();
		void OnWsWrite(boost::system::error_code ec, std::size_t bytes_transferred);

		void DoClose();
		void MarkDone();

	private:
		bool m_Done{ false };
		bool m_WsActive{ false };
		bool m_WsWriting{ false };

		bool m_HasSslContext{ false };
		boost::asio::ssl::context* m_SslContext{ nullptr };

		std::optional<TcpStream> m_TcpStream;
		std::optional<SslStream> m_SslStream;

		boost::beast::flat_buffer m_Buffer;
		std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> m_Parser;

		std::optional<WsStream> m_WsStream;
		std::optional<WsSslStream> m_WsSslStream;
		Interfaces::IWebSocketBase* m_WsHandler{ nullptr };
		boost::beast::flat_buffer m_WsReadBuffer;
		std::string m_WsWriteBuffer;

		static constexpr auto SESSION_TIMEOUT = std::chrono::seconds(30);
	};

	/// Async HTTP server.
	///
	/// Accepts connections via async_accept and creates HttpSessionState
	/// instances.  The main frame loop must call io_context.poll() to drive
	/// all async completions, and Poll() to kick WebSocket outbound writes.
	class HttpServer
	{
	public:
		HttpServer(boost::asio::io_context& io_context,
				   boost::asio::ip::tcp::endpoint endpoint,
				   std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context = std::nullopt);
		~HttpServer();

		HttpServer(const HttpServer&) = delete;
		HttpServer& operator=(const HttpServer&) = delete;

		bool Start();
		void Poll();
		void Stop();

	private:
		void DoAccept();
		void OnAccept(boost::system::error_code ec, TcpSocket socket);

		boost::asio::io_context& m_IoContext;
		boost::asio::ip::tcp::endpoint m_Endpoint;
		std::optional<TcpAcceptor> m_Acceptor;
		std::optional<std::reference_wrapper<boost::asio::ssl::context>> m_SslContext;

		std::vector<std::shared_ptr<HttpSessionState>> m_Sessions;
		bool m_Running{ false };

		// Security: Maximum concurrent connections to prevent resource exhaustion
		static constexpr std::size_t MAX_CONCURRENT_CONNECTIONS = 1000;
	};

}
// namespace AqualinkAutomate::HTTP
