#include <format>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include "http/server/http_server.h"
#include "http/server/routing/routing.h"
#include "http/server/responses/response_404.h"
#include "logging/logging.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "profiling/profiling_units/unit_colours.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	//=========================================================================
	// HttpSessionState
	//=========================================================================

	HttpSessionState::HttpSessionState(TcpSocket socket)
	{
		m_TcpStream.emplace(std::move(socket));
		m_TcpStream->expires_after(SESSION_TIMEOUT);
	}

	HttpSessionState::HttpSessionState(TcpSocket socket, boost::asio::ssl::context& ssl_ctx)
		: m_HasSslContext(true)
		, m_SslContext(&ssl_ctx)
	{
		m_TcpStream.emplace(std::move(socket));
		m_TcpStream->expires_after(SESSION_TIMEOUT);
	}

	void HttpSessionState::Start()
	{
		if (m_HasSslContext && m_SslContext)
		{
			DoSslHandshake();
		}
		else
		{
			DoRead();
		}
	}

	void HttpSessionState::Poll()
	{
		if (m_Done) return;

		if (m_WsActive && !m_WsWriting)
		{
			TryWsWrite();
		}
	}

	bool HttpSessionState::IsDone() const
	{
		return m_Done;
	}

	void HttpSessionState::Close()
	{
		if (m_Done) return;

		MarkDone();

		if (m_WsSslStream)
		{
			boost::beast::get_lowest_layer(*m_WsSslStream).cancel();
		}
		else if (m_WsStream)
		{
			boost::beast::get_lowest_layer(*m_WsStream).cancel();
		}
		else if (m_SslStream)
		{
			boost::beast::get_lowest_layer(*m_SslStream).cancel();
		}
		else if (m_TcpStream)
		{
			boost::beast::get_lowest_layer(*m_TcpStream).cancel();
		}
	}

	//--- SSL Handshake ---------------------------------------------------

	void HttpSessionState::DoSslHandshake()
	{
		if (!m_TcpStream || !m_SslContext)
		{
			MarkDone();
			return;
		}

		m_SslStream.emplace(std::move(*m_TcpStream), *m_SslContext);
		m_TcpStream.reset();

		boost::beast::get_lowest_layer(*m_SslStream).expires_after(SESSION_TIMEOUT);

		m_SslStream->async_handshake(
			boost::asio::ssl::stream_base::server,
			[self = shared_from_this()](boost::system::error_code ec)
			{
				self->OnSslHandshake(ec);
			});
	}

	void HttpSessionState::OnSslHandshake(boost::system::error_code ec)
	{
		if (m_Done) return;

		if (ec)
		{
			LogDebug(Channel::Web, std::format("SSL handshake failed: {}", ec.message()));
			MarkDone();
			return;
		}

		DoRead();
	}

	//--- HTTP Read --------------------------------------------------------

	void HttpSessionState::DoRead()
	{
		if (m_Done) return;

		m_Parser.emplace();
		m_Parser->body_limit(10000);

		if (m_SslStream)
		{
			boost::beast::get_lowest_layer(*m_SslStream).expires_after(SESSION_TIMEOUT);

			boost::beast::http::async_read(*m_SslStream, m_Buffer, *m_Parser,
				[self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnRead(ec, bytes);
				});
		}
		else if (m_TcpStream)
		{
			m_TcpStream->expires_after(SESSION_TIMEOUT);

			boost::beast::http::async_read(*m_TcpStream, m_Buffer, *m_Parser,
				[self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnRead(ec, bytes);
				});
		}
		else
		{
			MarkDone();
		}
	}

	void HttpSessionState::OnRead(boost::system::error_code ec, std::size_t /*bytes_transferred*/)
	{
		if (m_Done) return;

		if (ec == boost::beast::http::error::end_of_stream)
		{
			DoClose();
			return;
		}

		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				LogTrace(Channel::Web, std::format("HTTP read failed: {}", ec.message()));
			}
			MarkDone();
			return;
		}

		ProcessRequest();
	}

	//--- Process Request --------------------------------------------------

	void HttpSessionState::ProcessRequest()
	{
		if (!m_Parser)
		{
			MarkDone();
			return;
		}

		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HttpServer::handle_request", std::source_location::current());

		{
			auto method = std::string(m_Parser->get().method_string());
			auto target = std::string(m_Parser->get().target());
			Factory::ProfilerFactory::Instance().Get()->Message(
				std::format("HTTP: {} {}", method, target),
				static_cast<uint32_t>(Profiling::UnitColours::Cyan));
		}

		if (boost::beast::websocket::is_upgrade(m_Parser->get()))
		{
			zone->Text("WebSocket Upgrade");
			LogTrace(Channel::Web, "WebSocket upgrade requested");

			m_WsHandler = Routing::WS_OnAccept(m_Parser->get().target());
			if (!m_WsHandler)
			{
				LogDebug(Channel::Web, "No WebSocket handler found for target");
				auto req = m_Parser->release();
				DoWrite(HTTP::Responses::Response_404(req));
				return;
			}

			DoWsAccept(m_Parser->release());
			return;
		}

		zone->Text(std::string(m_Parser->get().target()));

		auto req = m_Parser->release();
		auto msg = Routing::HTTP_OnRequest(req);

		DoWrite(std::move(msg));
	}

	//--- HTTP Write -------------------------------------------------------

	void HttpSessionState::DoWrite(Message msg)
	{
		if (m_Done) return;

		bool keep_alive = msg.keep_alive();

		if (m_SslStream)
		{
			boost::beast::get_lowest_layer(*m_SslStream).expires_after(SESSION_TIMEOUT);

			boost::beast::async_write(*m_SslStream, std::move(msg),
				[self = shared_from_this(), keep_alive](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnWrite(ec, bytes, keep_alive);
				});
		}
		else if (m_TcpStream)
		{
			m_TcpStream->expires_after(SESSION_TIMEOUT);

			boost::beast::async_write(*m_TcpStream, std::move(msg),
				[self = shared_from_this(), keep_alive](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnWrite(ec, bytes, keep_alive);
				});
		}
		else
		{
			MarkDone();
		}
	}

	void HttpSessionState::OnWrite(boost::system::error_code ec, std::size_t bytes_transferred, bool keep_alive)
	{
		if (m_Done) return;

		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				LogDebug(Channel::Web, std::format("HTTP write failed: {}", ec.message()));
			}
			MarkDone();
			return;
		}

		LogTrace(Channel::Web, std::format("HTTP response written ({} bytes)", bytes_transferred));

		if (!keep_alive)
		{
			DoClose();
		}
		else
		{
			DoRead();
		}
	}

	//--- WebSocket Accept -------------------------------------------------

	void HttpSessionState::DoWsAccept(const Request& req)
	{
		if (m_Done) return;

		if (m_SslStream)
		{
			boost::beast::get_lowest_layer(*m_SslStream).expires_never();
			m_WsSslStream.emplace(std::move(*m_SslStream));
			m_SslStream.reset();

			m_WsSslStream->set_option(
				boost::beast::websocket::stream_base::timeout::suggested(
					boost::beast::role_type::server));

			m_WsSslStream->async_accept(req,
				[self = shared_from_this()](boost::system::error_code ec)
				{
					self->OnWsAccept(ec);
				});
		}
		else if (m_TcpStream)
		{
			m_TcpStream->expires_never();
			m_WsStream.emplace(std::move(*m_TcpStream));
			m_TcpStream.reset();

			m_WsStream->set_option(
				boost::beast::websocket::stream_base::timeout::suggested(
					boost::beast::role_type::server));

			m_WsStream->async_accept(req,
				[self = shared_from_this()](boost::system::error_code ec)
				{
					self->OnWsAccept(ec);
				});
		}
		else
		{
			MarkDone();
		}
	}

	void HttpSessionState::OnWsAccept(boost::system::error_code ec)
	{
		if (m_Done) return;

		if (ec)
		{
			LogDebug(Channel::Web, std::format("WebSocket accept failed: {}", ec.message()));
			MarkDone();
			return;
		}

		Factory::ProfilerFactory::Instance().Get()->Message("WebSocket: Upgrade accepted", static_cast<uint32_t>(Profiling::UnitColours::Cyan));
		m_WsConnectionId = m_WsHandler->OnOpen();
		m_WsActive = true;

		DoWsRead();
	}

	//--- WebSocket Read ---------------------------------------------------

	void HttpSessionState::DoWsRead()
	{
		if (m_Done) return;

		m_WsReadBuffer.clear();

		if (m_WsSslStream)
		{
			m_WsSslStream->async_read(m_WsReadBuffer,
				[self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnWsRead(ec, bytes);
				});
		}
		else if (m_WsStream)
		{
			m_WsStream->async_read(m_WsReadBuffer,
				[self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnWsRead(ec, bytes);
				});
		}
		else
		{
			MarkDone();
		}
	}

	void HttpSessionState::OnWsRead(boost::system::error_code ec, std::size_t bytes)
	{
		if (m_Done) return;

		if (ec == boost::beast::websocket::error::closed)
		{
			if (m_WsHandler) m_WsHandler->OnClose(m_WsConnectionId);
			MarkDone();
			return;
		}

		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				LogTrace(Channel::Web, std::format("WebSocket read error: {}", ec.message()));
				if (m_WsHandler) m_WsHandler->OnError(m_WsConnectionId);
			}
			MarkDone();
			return;
		}

		if (bytes > 0 && m_WsHandler)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HttpServer::ws_message", std::source_location::current());
			m_WsHandler->OnMessage(m_WsConnectionId, m_WsReadBuffer);
		}

		DoWsRead();
	}

	//--- WebSocket Write --------------------------------------------------

	void HttpSessionState::TryWsWrite()
	{
		if (m_Done || m_WsWriting || !m_WsHandler) return;

		auto msg = m_WsHandler->DequeueMessage(m_WsConnectionId);
		if (!msg) return;

		m_WsWriting = true;
		m_WsWriteBuffer = std::move(*msg);

		if (m_WsSslStream)
		{
			m_WsSslStream->binary(false);
			m_WsSslStream->async_write(boost::asio::buffer(m_WsWriteBuffer),
				[self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnWsWrite(ec, bytes);
				});
		}
		else if (m_WsStream)
		{
			m_WsStream->binary(false);
			m_WsStream->async_write(boost::asio::buffer(m_WsWriteBuffer),
				[self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
				{
					self->OnWsWrite(ec, bytes);
				});
		}
		else
		{
			m_WsWriting = false;
			MarkDone();
		}
	}

	void HttpSessionState::OnWsWrite(boost::system::error_code ec, std::size_t /*bytes*/)
	{
		m_WsWriting = false;

		if (m_Done) return;

		if (ec == boost::beast::websocket::error::closed)
		{
			if (m_WsHandler) m_WsHandler->OnClose(m_WsConnectionId);
			MarkDone();
			return;
		}

		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				LogDebug(Channel::Web, std::format("WebSocket write error: {}", ec.message()));
				if (m_WsHandler) m_WsHandler->OnError(m_WsConnectionId);
			}
			MarkDone();
			return;
		}

		if (m_WsHandler) m_WsHandler->OnPublish(m_WsConnectionId);

		// Chain: try writing the next queued message immediately
		TryWsWrite();
	}

	//--- Close / Cleanup --------------------------------------------------

	void HttpSessionState::DoClose()
	{
		if (m_Done) return;

		if (m_SslStream)
		{
			boost::beast::get_lowest_layer(*m_SslStream).expires_after(SESSION_TIMEOUT);

			m_SslStream->async_shutdown(
				[self = shared_from_this()](boost::system::error_code /*ec*/)
				{
					self->MarkDone();
				});
			return;
		}

		if (m_TcpStream)
		{
			boost::system::error_code ec;
			boost::beast::get_lowest_layer(*m_TcpStream).socket().shutdown(
				boost::asio::ip::tcp::socket::shutdown_send, ec);
		}

		MarkDone();
	}

	void HttpSessionState::MarkDone()
	{
		if (m_Done) return;

		m_Done = true;
		m_WsActive = false;
		m_WsHandler = nullptr;
	}

	//=========================================================================
	// HttpServer
	//=========================================================================

	HttpServer::HttpServer(boost::asio::io_context& io_context,
						   boost::asio::ip::tcp::endpoint endpoint,
						   std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context)
		: m_IoContext(io_context)
		, m_Endpoint(std::move(endpoint))
		, m_SslContext(ssl_context)
	{
	}

	HttpServer::~HttpServer()
	{
		Stop();
	}

	bool HttpServer::Start()
	{
		boost::system::error_code ec;

		m_Acceptor.emplace(m_IoContext);

		if (m_Acceptor->open(m_Endpoint.protocol(), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to open HTTP acceptor: {}", ec.message()));
			return false;
		}

		if (m_Acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to set reuse_address: {}", ec.message()));
			return false;
		}

		if (m_Acceptor->bind(m_Endpoint, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to bind to endpoint: {}", ec.message()));
			return false;
		}

		if (m_Acceptor->listen(boost::asio::socket_base::max_listen_connections, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to listen: {}", ec.message()));
			return false;
		}

		m_Running = true;
		LogInfo(Channel::Web, std::format("HTTP server listening on {}:{}", m_Endpoint.address().to_string(), m_Endpoint.port()));

		DoAccept();

		return true;
	}

	void HttpServer::DoAccept()
	{
		if (!m_Running || !m_Acceptor) return;

		m_Acceptor->async_accept(
			[this](boost::system::error_code ec, TcpSocket socket)
			{
				OnAccept(ec, std::move(socket));
			});
	}

	void HttpServer::OnAccept(boost::system::error_code ec, TcpSocket socket)
	{
		if (!m_Running) return;

		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				LogDebug(Channel::Web, std::format("Accept error: {}", ec.message()));
			}

			if (m_Running)
			{
				DoAccept();
			}
			return;
		}

		// Security: Enforce connection limit to prevent resource exhaustion
		// Clean up completed sessions first
		std::erase_if(m_Sessions, [](const auto& s) { return !s || s->IsDone(); });

		if (m_Sessions.size() >= MAX_CONCURRENT_CONNECTIONS)
		{
			LogWarning(Channel::Web, std::format("Connection limit ({}) reached, rejecting new connection", MAX_CONCURRENT_CONNECTIONS));
			boost::system::error_code close_ec;
			socket.close(close_ec);
			DoAccept();
			return;
		}

		LogInfo(Channel::Web, "Accepted new HTTP connection");
		Factory::ProfilerFactory::Instance().Get()->Message("New HTTP connection accepted");

		std::shared_ptr<HttpSessionState> session;

		if (m_SslContext)
		{
			session = std::make_shared<HttpSessionState>(std::move(socket), m_SslContext->get());
		}
		else
		{
			session = std::make_shared<HttpSessionState>(std::move(socket));
		}

		m_Sessions.push_back(session);
		session->Start();

		DoAccept();
	}

	void HttpServer::Poll()
	{
		if (!m_Running) return;

		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HttpServer::Poll", std::source_location::current());
		Factory::ProfilerFactory::Instance().Get()->PlotValue("Active HTTP Sessions", static_cast<int64_t>(m_Sessions.size()));

		// Kick WebSocket outbound writes
		for (auto& session : m_Sessions)
		{
			if (session && !session->IsDone())
			{
				session->Poll();
			}
		}

		// Remove completed sessions
		std::erase_if(m_Sessions, [](const auto& s) { return !s || s->IsDone(); });
	}

	void HttpServer::Stop() noexcept
	{
		m_Running = false;

		if (m_Acceptor && m_Acceptor->is_open())
		{
			boost::system::error_code ec;
			m_Acceptor->close(ec);
		}

		for (auto& session : m_Sessions)
		{
			if (session) session->Close();
		}

		m_Sessions.clear();

		LogInfo(Channel::Web, "HTTP server stopped");
	}

}
// namespace AqualinkAutomate::HTTP
