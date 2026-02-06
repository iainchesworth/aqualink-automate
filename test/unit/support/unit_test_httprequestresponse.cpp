#include <boost/test/unit_test.hpp>

#include <boost/beast.hpp>
#include <boost/beast/_experimental/test/handler.hpp>

#include "mocks/mock_beast_basicstream_with_timeout.h"
#include "http/server/http_server.h"
#include "http/server/routing/routing.h"
#include "support/unit_test_httprequestresponse.h"

namespace AqualinkAutomate::Test
{

	HTTP::Response PerformHttpRequestResponse(const std::string_view& url_to_retrieve)
	{
		HTTP::Request req;
		HTTP::Response resp;

		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target(url_to_retrieve);
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		// Use the routing system directly to process the request synchronously
		auto msg = HTTP::Routing::HTTP_OnRequest(req);

		// Extract the response from the message_generator
		// Since message_generator wraps the response, we need to serialize and deserialize
		boost::beast::flat_buffer buffer;
		boost::beast::error_code ec;

		// For test purposes, create a pair of connected test streams
		boost::asio::io_context ioc;
		auto exec = ioc.get_executor();

		Test::MockBeastBasicStreamWithTimeout client_stream(exec);
		Test::MockBeastBasicStreamWithTimeout server_stream(exec);

		server_stream.connect(client_stream);

		// Write the response to the server stream
		boost::beast::write(server_stream, std::move(msg), ec);

		if (ec)
		{
			throw std::runtime_error("Failed to write response: " + ec.message());
		}

		server_stream.close();

		// Read the response from the client stream
		ioc.poll();

		boost::beast::flat_buffer read_buffer;
		boost::beast::http::read(client_stream, read_buffer, resp, ec);

		if (ec && ec != boost::beast::http::error::end_of_stream)
		{
			throw std::runtime_error("Failed to read response: " + ec.message());
		}

		return resp;
	}

}
// namespace AqualinkAutomate::Test
