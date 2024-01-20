#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/http.hpp>

namespace AqualinkAutomate::Test
{

	class MockBeastBasicStreamWithTimeout : public boost::beast::test::stream
	{
	public:
		MockBeastBasicStreamWithTimeout(boost::asio::io_context& ioc);

	public:
		void expires_after(boost::asio::steady_timer::duration expiry_time);
		void expires_never();
	};

}
// namespace AqualinkAutomate::Test
