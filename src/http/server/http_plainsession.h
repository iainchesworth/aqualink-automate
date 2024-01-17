#pragma once

#include <format>
#include <memory>
#include <typeinfo>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/system/error_code.hpp>

#include "http/server/http_session.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{
	template<typename TCP_STREAM>
	class HTTP_PlainSession_Base : public HTTP_Session<HTTP_PlainSession_Base<TCP_STREAM>>, public std::enable_shared_from_this<HTTP_PlainSession_Base<TCP_STREAM>>
	{
	public:
		HTTP_PlainSession_Base(TCP_STREAM&& stream, boost::beast::flat_buffer&& buffer) :
			HTTP_Session<HTTP_PlainSession_Base<TCP_STREAM>>(std::move(buffer)),
			m_Stream(std::move(stream))
		{
			LogTrace(Channel::Web, std::format("HTTP_PlainSession object created using TCP_STREAM ({})", typeid(TCP_STREAM).name()));
		}

	public:
		void Run()
		{
			HTTP_Session<HTTP_PlainSession_Base<TCP_STREAM>>::DoRead();
		}

	public:
		TCP_STREAM& Stream()
		{
			return m_Stream;
		}

		TCP_STREAM ReleaseStream()
		{
			return std::move(m_Stream);
		}

	public:
		void DoEOF()
		{
			boost::system::error_code ec;

			if (m_Stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec); ec)
			{
				LogTrace(Channel::Web, std::format("Failed to shutdown tcp socket; error was -> {}", ec.message()));
			}
		}

	private:
		TCP_STREAM m_Stream;
	};

	using HTTP_PlainSession = HTTP_PlainSession_Base<boost::beast::tcp_stream>;

}
// namespace AqualinkAutomate::HTTP
