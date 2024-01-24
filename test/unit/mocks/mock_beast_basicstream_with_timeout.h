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

#include "http/server/websocket_plainsession.h"

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

	template<class BODY, class ALLOCATOR>
	void WebSocket_MakeSession(MockBeastBasicStreamWithTimeout stream, boost::beast::http::request<BODY, boost::beast::http::basic_fields<ALLOCATOR>> req)
	{
		using WebSocket_PlainSession_Test = HTTP::WebSocket_PlainSession_Base<boost::beast::test::stream>;

		std::make_shared<WebSocket_PlainSession_Test>(std::move(stream))->Run(std::move(req));
	}

}
// namespace AqualinkAutomate::Test
