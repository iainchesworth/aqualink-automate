#pragma once

#include <format>
#include <memory>
#include <string>

#include <boost/asio/ssl/error.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "http/server/router.h"
#include "interfaces/isession.h"

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

            SessionType().WS().async_accept(
                req,
                [this, self = SessionType().shared_from_this()](boost::system::error_code ec)
                {
                    switch (ec.value())
                    {
                    case boost::system::errc::success:
                        DoRead();
                        break;

                    case boost::asio::ssl::error::stream_truncated:
                        [[fallthrough]];
                    default:
                        LogDebug(Channel::Web, std::format("Failed during accept of WebSocket stream; error was -> {}", ec.message()));
                        break;
                    }
                }
            );
        }

    private:
        void DoRead()
        {
            SessionType().WS().async_read(
                m_Buffer,
                [this, self = SessionType().shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
                {
                    boost::ignore_unused(bytes_transferred);

                    switch (ec.value())
                    {
                    case boost::system::errc::success:
                        SessionType().WS().text(SessionType().WS().got_text());
                        SessionType().WS().async_write(
                            m_Buffer.data(),
                            [this, self = SessionType().shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred)
                            {
                                boost::ignore_unused(bytes_transferred);

                                switch (ec.value())
                                {
                                case boost::system::errc::success:
                                    m_Buffer.consume(m_Buffer.size());
                                    DoRead();
                                    break;

                                default:
                                    LogDebug(Channel::Web, std::format("Failed during write of WebSocket stream; error was -> {}", ec.message()));
                                    break;
                                }
                            }
                        );
                        break;

                    case boost::asio::ssl::error::stream_truncated:
                        [[fallthrough]];
                    default:
                        LogDebug(Channel::Web, std::format("Failed during read of WebSocket stream; error was -> {}", ec.message()));
                        break;
                    }
                }
            );
        }

    private:
        boost::beast::flat_buffer m_Buffer;
	};

}
// namespace AqualinkAutomate::HTTP
