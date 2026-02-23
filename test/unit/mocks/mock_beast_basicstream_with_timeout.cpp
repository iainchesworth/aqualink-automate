#include "mocks/mock_beast_basicstream_with_timeout.h"

namespace AqualinkAutomate::Test
{
	
	MockBeastBasicStreamWithTimeout::MockBeastBasicStreamWithTimeout(const boost::asio::any_io_executor& exec) :
		boost::beast::test::stream(exec),
		m_Timeout(exec)
	{
	}

	void MockBeastBasicStreamWithTimeout::cancel()
	{
		boost::system::error_code ec;
		m_Timeout.cancel();
	}

	void MockBeastBasicStreamWithTimeout::expires_after(boost::asio::steady_timer::duration expiry_time)
	{
		boost::system::error_code ec;

		m_Timeout.expires_after(expiry_time);
		m_Timeout.async_wait(
			[this](const boost::system::error_code& tec) {
				if (!tec) 
				{
					this->cancel();
				}
			});
	}

	void MockBeastBasicStreamWithTimeout::expires_never()
	{
		m_Timeout.cancel();
	}

	void teardown(boost::beast::role_type role, AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream, boost::beast::error_code& ec)
	{
		boost::beast::test::teardown(role, static_cast<boost::beast::test::stream&>(stream), ec);
	}

	void teardown(boost::beast::role_type role, boost::beast::websocket::stream<AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout, true>& stream, boost::beast::error_code& ec)
	{
		throw;
	}

}
// namespace AqualinkAutomate::Test

namespace AqualinkAutomate::HTTP
{

	void CloseStream(AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout& stream)
	{
		stream.expires_never();
		stream.close_remote();
		stream.close();
	}

}
// namespace AqualinkAutomate::HTTP

