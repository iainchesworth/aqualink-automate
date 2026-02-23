#include <boost/test/unit_test.hpp>

#include <boost/beast.hpp>
#include <boost/beast/_experimental/test/handler.hpp>

#include "mocks/mock_beast_basicstream_with_timeout.h"
#include "http/server/http_server.h"
#include "http/server/routing/routing.h"
#include "support/unit_test_wsrequestresponse.h"

namespace AqualinkAutomate::Test
{

	std::tuple<std::array<uint8_t, 1024>, std::size_t> PerformHttpWsUpgradeResponse(const std::string_view& url_to_retrieve, std::function<void()> on_ws_connected)
	{
		std::array<uint8_t, 1024> buffer{};
		std::size_t bytes_read = 0;

		// WebSocket upgrade testing is not yet supported in the poll-based architecture.
		// The old coroutine-based implementation was already a no-op (immediate co_return).

		return { buffer, bytes_read };
	}

}
// namespace AqualinkAutomate::Test
