#include "mocks/mock_beast_basicstream_with_timeout.h"

namespace AqualinkAutomate::Test
{
	
	MockBeastBasicStreamWithTimeout::MockBeastBasicStreamWithTimeout(boost::asio::io_context& ioc) :
		boost::beast::test::stream(ioc)
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
