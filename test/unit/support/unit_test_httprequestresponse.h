#pragma once

#include <string_view>
#include <tuple>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/_experimental/test/tcp.hpp>

#include "http/server/http_plainsession.h"
#include "http/server/server_types.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Test
{
	using HTTP_PlainSession_Test = HTTP::HTTP_PlainSession_Base<Test::MockBeastBasicStreamWithTimeout>;

	struct TestResponseObject
	{
	private:
		boost::asio::io_context& internal_ioc;

	public:
		Test::MockBeastBasicStreamWithTimeout local_stream;
		std::shared_ptr<HTTP_PlainSession_Test> session_ptr;
		
		boost::beast::flat_buffer request_buffer;
		HTTP::Request last_request;

		boost::beast::flat_buffer response_buffer;
		HTTP::Response last_response;

		TestResponseObject(boost::asio::io_context& ioc, Test::MockBeastBasicStreamWithTimeout& remote_stream) :
			internal_ioc(ioc),
			local_stream(ioc),
			session_ptr(nullptr),
			request_buffer(),
			last_request(),
			response_buffer(),
			last_response()
		{
			local_stream.connect(remote_stream);
		}

		TestResponseObject(const TestResponseObject&) = delete;
		TestResponseObject(TestResponseObject&&) noexcept = delete;

		~TestResponseObject()
		{
			local_stream.close_remote();
			local_stream.close();

			if (nullptr != session_ptr)
			{
				session_ptr->Stop();
			}
		}
	};

	std::shared_ptr<TestResponseObject> PerformHttpRequestResponse(boost::asio::io_context& io_context, const std::string_view& url_to_retrieve);
	std::shared_ptr<TestResponseObject> PerformHttpWsUpgradeResponse(boost::asio::io_context& io_context, const std::string_view& url_to_retrieve);

}
// namespace AqualinkAutomate::Test
