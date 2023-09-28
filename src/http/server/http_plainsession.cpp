#include <boost/system/error_code.hpp>

#include "http/server/http_plainsession.h"

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

		m_Stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
	}

}
// namespace AqualinkAutomate::HTTP
