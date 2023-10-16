#pragma once

#include <format>
#include <memory>
#include <string>
#include <vector>

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
        template<class BODY, class ALLOCATOR>
        void Run(boost::beast::http::request<BODY, boost::beast::http::basic_fields<ALLOCATOR>> req)
        {
            DoAccept(std::move(req));
        }

	private:
        template<class Body, class Allocator>
        void DoAccept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
        {
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
                );
            }
        }

    private:
        void DoRead()
        {
            SessionType().WS().async_read(
                m_Buffer,
                [this, self = SessionType().shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
                {
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
            );
        }

    private:
        void DoWrite()
        {
            auto& [buffer_ptr, is_binary] = m_ResponseQueue.front();

            SessionType().WS().binary(is_binary);

            SessionType().WS().async_write(
                boost::asio::buffer(*buffer_ptr),
                [this, self = SessionType().shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
                {
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
                        m_ResponseQueue.erase(m_ResponseQueue.begin());

                        // Send the next message, if any
                        if (!m_ResponseQueue.empty())
                        {
                            DoWrite();
                        }
                    }
                }
            );
        }

    public:
        void QueueWrite(std::shared_ptr<const std::string> message, bool is_binary)
        {
            m_ResponseQueue.push_back({ message, is_binary });
                    
            if (1 == m_ResponseQueue.size())
            {
                LogTrace(Channel::Web, "Response queue has a queued response in it so commence response write loop");
                DoWrite();
            }
        }

    private:
        using ResponseQueueBufferType = std::tuple<std::shared_ptr<const std::string>, bool>;
        std::vector<ResponseQueueBufferType> m_ResponseQueue;

    private:
        Interfaces::IWebSocketBase* m_Handler_WeakPtr{ nullptr };
        boost::beast::flat_buffer m_Buffer{};
    };

}
// namespace AqualinkAutomate::HTTP
