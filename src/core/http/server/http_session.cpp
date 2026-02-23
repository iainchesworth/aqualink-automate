#include <boost/asio.hpp>
#include <boost/cobalt/promise.hpp>

#include "http/server/http_session.h"

namespace AqualinkAutomate::HTTP
{

	boost::cobalt::promise<void> CloseStream(TcpStream& stream)
	{
		// This is only for non-SSL streams
		boost::beast::error_code ec;
		boost::beast::get_lowest_layer(stream).socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

		co_return;
	}

	boost::cobalt::promise<void> CloseStream(SslStream& stream)
	{
		// This is only for SSL streams.
		auto [ec] = co_await stream.async_shutdown(boost::asio::as_tuple(boost::cobalt::use_op));

		co_return;
	}

}
// namespace AqualinkAutomate::HTTP
