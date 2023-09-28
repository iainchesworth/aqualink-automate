#pragma once 

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

#include "http/server/router.h"
#include "http/server/websocket_session_helper.h"
#include "interfaces/isession.h"

namespace AqualinkAutomate::HTTP
{
	
	template<class SESSION_TYPE>
	class HTTP_Session : public Interfaces::ISession
	{
		static constexpr std::size_t QUEUE_LIMIT = 8;

	private:
		SESSION_TYPE& SessionType()
		{
			return static_cast<SESSION_TYPE&>(*this);
		}

	public:
        HTTP_Session(boost::beast::flat_buffer buffer, std::shared_ptr<Router> router) :
			m_Router(router),
			m_Buffer(std::move(buffer))
		{
		}

	public:
		void DoRead()
		{
			m_Parser.emplace();
			m_Parser->body_limit(10000);

			boost::beast::get_lowest_layer(SessionType().Stream()).expires_after(std::chrono::seconds(30));

			boost::beast::http::async_read(
				SessionType().Stream(),
				m_Buffer,
				*m_Parser,
				[this, self = SessionType().shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
				{
					boost::ignore_unused(bytes_transferred);

					switch (ec.value())
					{
                    case boost::system::errc::success:
						if (boost::beast::websocket::is_upgrade(m_Parser->get()))
						{
							boost::beast::get_lowest_layer(SessionType().Stream()).expires_never();
							return WebSocket_MakeSession(SessionType().ReleaseStream(), m_Parser->release());
						}
						else
						{
                            QueueWrite(m_Router->HTTP_OnRequest(m_Parser->release()));

							if (QUEUE_LIMIT > m_ResponseQueue.size())
							{
								DoRead();
							}
						}
						break;

					case boost::asio::ssl::error::stream_truncated:
						break;

					default:
						break;
					}
				}
			);
		}

		bool DoWrite()
		{
			bool const was_full = (QUEUE_LIMIT == m_ResponseQueue.size());

			if (!m_ResponseQueue.empty())
			{
				boost::beast::http::message_generator msg = std::move(m_ResponseQueue.front());
				m_ResponseQueue.erase(m_ResponseQueue.begin());

				bool keep_alive = msg.keep_alive();

				boost::beast::async_write(
					SessionType().Stream(),
					std::move(msg),
					[this, self = SessionType().shared_from_this(), keep_alive](boost::system::error_code ec, std::size_t bytes_transferred)
					{
						boost::ignore_unused(bytes_transferred);

						switch (ec.value())
						{
                        case boost::system::errc::success:
							if (!keep_alive)
							{
								return SessionType().DoEOF();
							}
							else if (DoWrite())
							{
								DoRead();
							}
							else
							{
								//FIXME
							}
							break;

						case boost::asio::ssl::error::stream_truncated:
							break;

						default:
							break;
						}
					}
				);
			}

			return was_full;
		}

	public:
		void QueueWrite(boost::beast::http::message_generator response)
		{
			m_ResponseQueue.push_back(std::move(response));

			if (1 == m_ResponseQueue.size())
			{
				DoWrite();
			}
		}

	protected:
		boost::beast::flat_buffer m_Buffer;

	private:
		std::shared_ptr<Router> m_Router;
		std::vector<boost::beast::http::message_generator> m_ResponseQueue;
		std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> m_Parser;
	};

}
// namespace AqualinkAutomate::HTTP
