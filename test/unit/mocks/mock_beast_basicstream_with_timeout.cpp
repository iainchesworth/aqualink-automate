#include "mocks/mock_beast_basicstream_with_timeout.h"

namespace AqualinkAutomate::Test
{
	
	MockBeastBasicStreamWithTimeout::MockBeastBasicStreamWithTimeout(boost::asio::io_context& ioc) :
		boost::beast::test::stream(ioc)
	{
	}

	void MockBeastBasicStreamWithTimeout::cancel()
	{
	}

	void MockBeastBasicStreamWithTimeout::expires_after(boost::asio::steady_timer::duration expiry_time)
	{
	}

	void MockBeastBasicStreamWithTimeout::expires_never()
	{
	}

}
// namespace AqualinkAutomate::Test

namespace boost::beast::websocket
{

	void teardown(boost::beast::role_type role, AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream, boost::beast::error_code& ec)
	{
	}

}
// namespace boost::beast::websocket
