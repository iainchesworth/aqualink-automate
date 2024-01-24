#pragma once 

#include <chrono>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/system/error_code.hpp>

#include "http/server/websocket_session_helper.h"
#include "http/server/routing/routing.h"
#include "interfaces/isession.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{
	
	template<class SESSION_TYPE>
	class HTTP_Session : public Interfaces::ISession
	{
	private:
		SESSION_TYPE& SessionType()
		{
			return static_cast<SESSION_TYPE&>(*this);
		}

	public:
        HTTP_Session(boost::beast::flat_buffer buffer) :
			Interfaces::ISession(),
			m_Buffer(std::move(buffer))
		{
			LogTrace(Channel::Web, std::format("Creating HTTP session: {}", boost::uuids::to_string(Id())));
		}

		virtual ~HTTP_Session()
		{
			LogTrace(Channel::Web, std::format("Destroying HTTP session: {}", boost::uuids::to_string(Id())));

			m_CancelRead.emit(boost::asio::cancellation_type::terminal);
			m_CancelWrite.emit(boost::asio::cancellation_type::terminal);
		}

	public:
		virtual void Stop() override
		{
			LogTrace(Channel::Web, std::format("HTTP session {}: Stop()", boost::uuids::to_string(Id())));
			m_CancelRead.emit(boost::asio::cancellation_type::partial);
			m_CancelWrite.emit(boost::asio::cancellation_type::partial);
			m_Buffer.clear();
		}

	private:
		virtual void DoReadImpl() override
		{
			LogTrace(Channel::Web, std::format("HTTP session {}: DoReadImpl()", boost::uuids::to_string(Id())));

			m_Parser.emplace();
			m_Parser->body_limit(10000);

			boost::beast::get_lowest_layer(SessionType().Stream()).expires_after(std::chrono::seconds(30));

			boost::beast::http::async_read(
				SessionType().Stream(),
				m_Buffer,
				*m_Parser,
				boost::asio::bind_cancellation_slot(
					m_CancelRead.slot(),
					[this, self = SessionType().shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
					{
						LogTrace(Channel::Web, std::format("HTTP session {}: DoReadImpl()->Handler", boost::uuids::to_string(Id())));

						boost::ignore_unused(bytes_transferred);

						if (boost::beast::http::error::end_of_stream == ec)
						{
							return SessionType().DoEOF();
						} 
						else if (ec)
						{
							LogTrace(Channel::Web, std::format("Failed during read of HTTP stream; error was -> {}", ec.message()));
						} 
						else if (boost::beast::websocket::is_upgrade(m_Parser->get()))
						{
							LogTrace(Channel::Web, "WebSocket upgrade requested detected on HTTP stream -> transitioning to WebSockets");
							boost::beast::get_lowest_layer(SessionType().Stream()).expires_never();
							return WebSocket_MakeSession(SessionType().ReleaseStream(), m_Parser->release());
						}
						else
						{
							LogTrace(Channel::Web, "HTTP request from client; handling request");
							QueueWrite(std::move(Routing::HTTP_OnRequest(m_Parser->release())));

							if (QUEUE_LIMIT > m_ResponseQueue.size())
							{
								DoRead();
							}
						}
					}
				)
			);
		}

		virtual void DoWriteImpl() override
		{
			LogTrace(Channel::Web, std::format("HTTP session {}: DoWriteImpl()", boost::uuids::to_string(Id())));

			if (m_ResponseQueue.empty())
			{
				LogTrace(Channel::Web, "Response queue is empty -> stopping active write loop");
			}
			else
			{
				LogTrace(Channel::Web, std::format("Response queue contains {} elements to send -> queuing async write of first time", m_ResponseQueue.size()));

				try
				{
					auto payload = std::move(m_ResponseQueue.front());

					LogTrace(Channel::Web, std::format("HTTP session {}: DoWriteImpl()->Removing This Write ({} total)", boost::uuids::to_string(Id()), m_ResponseQueue.size()));

					m_ResponseQueue.erase(m_ResponseQueue.begin());

					auto&& msg = std::move(std::get<boost::beast::http::message_generator>(payload));

					bool keep_alive = msg.keep_alive();

					boost::beast::async_write(
						SessionType().Stream(),
						std::move(msg),
						boost::asio::bind_cancellation_slot(
							m_CancelWrite.slot(),
							[this, self = SessionType().shared_from_this(), keep_alive](boost::system::error_code ec, std::size_t bytes_transferred)
							{
								LogTrace(Channel::Web, std::format("HTTP session {}: DoWriteImpl()->Handler", boost::uuids::to_string(Id())));

								boost::ignore_unused(bytes_transferred);

								if (ec)
								{
									LogDebug(Channel::Web, std::format("Failed during write of HTTP stream; error was -> {}", ec.message()));
								}
								else if (!keep_alive)
								{
									LogTrace(Channel::Web, "Write completed; no keep-alive required so transitioning to EOF and closing stream");
									SessionType().DoEOF();
								}
								else if (DoWrite())
								{
									LogTrace(Channel::Web, "Write completed; keep-alive active and response queue is not full so queuing another read");
									DoRead();
								}
							}
						)
					);
				}
				catch (const std::bad_variant_access& ex)
				{
					LogDebug(Channel::Web, "Response queue held a response object of a type that cannot be processed");
				}
			}
		}

	protected:
		boost::beast::flat_buffer m_Buffer;

	private:
		std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> m_Parser;
		boost::asio::cancellation_signal m_CancelRead{}, m_CancelWrite{};
	};

}
// namespace AqualinkAutomate::HTTP
