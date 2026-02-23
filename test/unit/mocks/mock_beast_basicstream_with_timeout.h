#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
namespace AqualinkAutomate::Test
{

	class MockBeastBasicStreamWithTimeout : public boost::beast::test::stream
	{
	public:
		MockBeastBasicStreamWithTimeout(const boost::asio::any_io_executor& exec);

	public:
		void cancel();
		void expires_after(boost::asio::steady_timer::duration expiry_time);
		void expires_never();

	private:
		boost::asio::steady_timer m_Timeout;
	};

	void teardown(boost::beast::role_type role, AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream, boost::beast::error_code& ec);
	void teardown(boost::beast::role_type role, boost::beast::websocket::stream<AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout, true>& stream, boost::beast::error_code& ec);

	template<class TeardownHandler>
	void async_teardown(boost::beast::role_type role, AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream, TeardownHandler&& handler)
	{
		boost::beast::error_code ec;
		teardown(role, stream, ec);
		auto ex = stream.get_executor();
		boost::asio::post(ex, [h = std::forward<TeardownHandler>(handler), ec]() mutable { h(ec); });
	}
	template<class TeardownHandler>
	void async_teardown(boost::beast::role_type role, boost::beast::websocket::stream<AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout, true>& stream, TeardownHandler&& handler)
	{
		throw;
	}
}
// namespace AqualinkAutomate::Test

namespace AqualinkAutomate::HTTP
{

	void CloseStream(AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream);

}
// namespace AqualinkAutomate::HTTP

