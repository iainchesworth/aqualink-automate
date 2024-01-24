#pragma once

#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "formatters/beast_stringview_formatter.h"
#include "http/server/routing/routing.h"
#include "interfaces/isession.h"
#include "interfaces/iwebsocket.h"
#include "logging/logging.h"
#include "utility/overloaded_variant_visitor.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	template<class SESSION_TYPE>
	class WebSocket_Session : public Interfaces::ISession
	{
		SESSION_TYPE& SessionType()
		{
			return static_cast<SESSION_TYPE&>(*this);
		}

	public:
		WebSocket_Session() : 
			Interfaces::ISession()
		{
			LogTrace(Channel::Web, std::format("Creating WebSocket session: {}", boost::uuids::to_string(Id())));
		}

		virtual ~WebSocket_Session()
		{
			LogTrace(Channel::Web, std::format("Destroying WebSocket session: {}", boost::uuids::to_string(Id())));

			m_CancelAccept.emit(boost::asio::cancellation_type::terminal);
			m_CancelRead.emit(boost::asio::cancellation_type::terminal);
			m_CancelWrite.emit(boost::asio::cancellation_type::terminal);
		}

	public:
		template<class BODY, class ALLOCATOR>
		void Run(boost::beast::http::request<BODY, boost::beast::http::basic_fields<ALLOCATOR>> req)
		{
			LogTrace(Channel::Web, std::format("WebSocket session {}: Run()", boost::uuids::to_string(Id())));
			DoAccept(std::move(req));
		}

	public:
		virtual void Stop() override
		{
			LogTrace(Channel::Web, std::format("WebSocket session {}: Stop()", boost::uuids::to_string(Id())));
			m_CancelAccept.emit(boost::asio::cancellation_type::partial);
			m_CancelRead.emit(boost::asio::cancellation_type::partial);
			m_CancelWrite.emit(boost::asio::cancellation_type::partial);
			m_Buffer.clear();
		}

	private:
		template<class Body, class Allocator>
		void DoAccept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
		{
			LogTrace(Channel::Web, std::format("WebSocket session {}: DoAccept()", boost::uuids::to_string(Id())));

			SessionType().WS().set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
			SessionType().WS().set_option(
				boost::beast::websocket::stream_base::decorator(
					[](boost::beast::websocket::response_type& res)
					{
						res.set(
							boost::beast::http::field::server,
							std::string(BOOST_BEAST_VERSION_STRING) + " advanced-server-flex"
						);
					}
				)
			);

			if (m_Handler_WeakPtr = Routing::WS_OnAccept(req.target()); nullptr == m_Handler_WeakPtr)
			{
				LogDebug(Channel::Web, std::format("Could not find a suitable route handler for {}; ignoring request", req.target()));
			}
			else
			{
				SessionType().WS().async_accept(
					req,
					boost::asio::bind_cancellation_slot(
						m_CancelAccept.slot(),
						[this, self = SessionType().shared_from_this()](boost::system::error_code ec)
						{
							if (ec)
							{
								LogDebug(Channel::Web, std::format("Failed during accept of WebSocket stream; error was -> {}", ec.message()));
							}
							else
							{
								if (nullptr == m_Handler_WeakPtr)
								{
									LogDebug(Channel::Web, "Invalid session pointer; no OnOpen handler being called");
								}
								else
								{
									m_Handler_WeakPtr->Handle_OnOpen(self);
									DoRead();
								}
							}
						}
					)
				);
			}
		}

	private:
		virtual void DoReadImpl() override
		{
			LogTrace(Channel::Web, std::format("WebSocket session {}: DoReadImpl()", boost::uuids::to_string(Id())));

			SessionType().WS().async_read(
				m_Buffer,
				boost::asio::bind_cancellation_slot(
					m_CancelRead.slot(), 
					[this, self = SessionType().shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
					{
						LogTrace(Channel::Web, std::format("WebSocket session {}: DoReadImpl()->Handler", boost::uuids::to_string(Id())));

						boost::ignore_unused(bytes_transferred);

						if (nullptr == m_Handler_WeakPtr)
						{
							LogDebug(Channel::Web, "Invalid session pointer; ignoring read i.e. no OnMessage handler being called");
						}
						else if (boost::beast::websocket::error::closed == ec)
						{
							LogTrace(Channel::Web, "WebSocket was closed (during read); calling OnClose handler");
							m_Handler_WeakPtr->Handle_OnClose(self);
						}
						else if (ec)
						{
							LogTrace(Channel::Web, std::format("Failed during read of WebSocket stream; error was -> {}", ec.message()));
							m_Handler_WeakPtr->Handle_OnError(self);
						}
						else
						{
							m_Handler_WeakPtr->Handle_OnMessage(self, m_Buffer);
							DoRead();
						}
					}
				)
			);
		}

		virtual void DoWriteImpl() override
		{
			LogTrace(Channel::Web, std::format("WebSocket session {}: DoWriteImpl()", boost::uuids::to_string(Id())));

			auto async_write_remove_and_kick =
				[this]()
				{
					LogTrace(Channel::Web, std::format("WebSocket session {}: DoWriteImpl()->Handler->RemoveAndKick", boost::uuids::to_string(Id())));

					LogTrace(Channel::Web, std::format("WebSocket session {}: DoWriteImpl()->Handler->RemoveAndKick->Removing This Write ({} total)", boost::uuids::to_string(Id()), m_ResponseQueue.size()));

					m_ResponseQueue.erase(m_ResponseQueue.begin());

					// Send the next message, if any
					if (!m_ResponseQueue.empty())
					{
						LogTrace(Channel::Web, std::format("WebSocket session {}: DoWriteImpl()->Handler->RemoveAndKick->Triggering Next Write ({} remaining)", boost::uuids::to_string(Id()), m_ResponseQueue.size()));

						DoWrite();
					}
				};

			auto async_write_handler = 
				[this, async_write_remove_and_kick, self = SessionType().shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
				{
					LogTrace(Channel::Web, std::format("WebSocket session {}: DoWriteImpl()->Handler", boost::uuids::to_string(Id())));

					boost::ignore_unused(bytes_transferred);

					if (nullptr == m_Handler_WeakPtr)
					{
						LogDebug(Channel::Web, "Invalid session pointer; ignoring read i.e. no handlers will be called");
						m_ResponseQueue.clear();
					}
					else if (boost::beast::websocket::error::closed == ec)
					{
						LogTrace(Channel::Web, "WebSocket was closed (during write); calling OnClose handler");
						m_Handler_WeakPtr->Handle_OnClose(self);
						m_ResponseQueue.clear();
					}
					else if (ec)
					{
						LogDebug(Channel::Web, std::format("Failed during write of WebSocket stream; error was -> {}", ec.message()));
						m_Handler_WeakPtr->Handle_OnError(self);
						m_ResponseQueue.clear();
					}
					else
					{
						async_write_remove_and_kick();
					}
				};

			std::visit(
				Utility::OverloadedVisitor
				{
					[this, async_write_remove_and_kick](std::monostate)
					{
						LogDebug(Channel::Web, "Invalid payload type (non-binary, non-text) being sent over websocket");
						async_write_remove_and_kick();

					},
					[this, async_write_handler](const ResponseQueueTextElement& text_data)
					{
						SessionType().WS().binary(false);
						SessionType().WS().async_write(boost::asio::buffer(text_data), boost::asio::bind_cancellation_slot(m_CancelWrite.slot(), async_write_handler));
					},
					[this, async_write_handler](const ResponseQueueBinaryElement& binary_data)
					{
						SessionType().WS().binary(true);
						SessionType().WS().async_write(boost::asio::buffer(binary_data), boost::asio::bind_cancellation_slot(m_CancelWrite.slot(), async_write_handler));
					},
					[this, async_write_remove_and_kick](const ResponseQueueMessageElement&)
					{
						LogDebug(Channel::Web, "Invalid payload type (non-binary, non-text) being sent over websocket");
						async_write_remove_and_kick();
					}
				},
				m_ResponseQueue.front()
			);
		}

	private:
		boost::asio::cancellation_signal m_CancelRead{}, m_CancelWrite{}, m_CancelAccept{};
		Interfaces::IWebSocketBase* m_Handler_WeakPtr{ nullptr };
		boost::beast::flat_buffer m_Buffer{};
	};

}
// namespace AqualinkAutomate::HTTP
