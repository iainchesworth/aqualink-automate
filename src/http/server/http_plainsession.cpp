#include <format>

#include <boost/system/error_code.hpp>

#include "http/server/http_plainsession.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	HTTP_PlainSession::HTTP_PlainSession(boost::beast::tcp_stream&& stream, boost::beast::flat_buffer&& buffer, std::shared_ptr<Router> router) :
		HTTP_Session<HTTP_PlainSession>(std::move(buffer), router),
		m_Stream(std::move(stream))
	{
	}

	void HTTP_PlainSession::Run()
	{
		DoRead();
	}

	boost::beast::tcp_stream& HTTP_PlainSession::Stream()
	{
		return m_Stream;
	}

	boost::beast::tcp_stream HTTP_PlainSession::ReleaseStream()
	{
		return std::move(m_Stream);
	}

	void HTTP_PlainSession::DoEOF()
	{
		boost::system::error_code ec;

		if (m_Stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec); ec)
		{
			LogTrace(Channel::Web, std::format("Failed to shutdown tcp socket; error was -> {}", ec.message()));
		}
	}

}
// namespace AqualinkAutomate::HTTP
