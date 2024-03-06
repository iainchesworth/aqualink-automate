#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <boost/asio/async_result.hpp>
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
		void cancel();
		void expires_after(boost::asio::steady_timer::duration expiry_time);
		void expires_never();
	};

}
// namespace AqualinkAutomate::Test

namespace boost::beast::websocket
{

	void teardown(boost::beast::role_type role, AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream, boost::beast::error_code& ec);

	template<class TeardownHandler>
	void async_teardown(boost::beast::role_type role, AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream, TeardownHandler&& handler)
	{
	}

}
// namespace boost::beast::websocket
