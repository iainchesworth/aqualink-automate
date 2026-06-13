#include <chrono>
#include <exception>
#include <optional>
#include <source_location>
#include <string>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <nlohmann/json.hpp>

#include "http/server/make_response.h"
#include "http/server/responses/response_405.h"
#include "http/server/server_types.h"
#include "http/webroute_diagnostics_matter.h"
#include "profiling/factories/profiling_unit_factory.h"

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace AqualinkAutomate::HTTP
{
	namespace
	{
		constexpr std::chrono::milliseconds SIDECAR_TIMEOUT{ 750 };

		// Fetch the sidecar's /matter/status over localhost on a throwaway io_context,
		// bounded so a hung/absent sidecar cannot stall the main loop. Returns the
		// response body, or nullopt if it could not be reached in time.
		std::optional<std::string> FetchSidecarStatus(uint16_t status_port)
		{
			try
			{
				asio::io_context ioc;
				std::optional<std::string> body;

				asio::co_spawn(
					ioc,
					[status_port, &body]() -> asio::awaitable<void>
					{
						auto executor = co_await asio::this_coro::executor;
						tcp::resolver resolver(executor);
						beast::tcp_stream stream(executor);

						auto results = co_await resolver.async_resolve("127.0.0.1", std::to_string(status_port), asio::use_awaitable);
						stream.expires_after(SIDECAR_TIMEOUT);
						co_await stream.async_connect(results, asio::use_awaitable);

						http::request<http::string_body> req{ http::verb::get, "/matter/status", 11 };
						req.set(http::field::host, "127.0.0.1");
						req.set(http::field::user_agent, "aqualink-automate");
						stream.expires_after(SIDECAR_TIMEOUT);
						co_await http::async_write(stream, req, asio::use_awaitable);

						beast::flat_buffer buffer;
						http::response<http::string_body> res;
						co_await http::async_read(stream, buffer, res, asio::use_awaitable);
						body = res.body();

						beast::error_code ec;
						stream.socket().shutdown(tcp::socket::shutdown_both, ec);
					},
					asio::detached);

				// Bound the whole exchange; a refused localhost connection fails fast.
				ioc.run_for(SIDECAR_TIMEOUT + std::chrono::milliseconds(250));
				return body;
			}
			catch (const std::exception&)
			{
				return std::nullopt;
			}
		}
	}
	// unnamed namespace

	WebRoute_Diagnostics_Matter::WebRoute_Diagnostics_Matter(bool enabled, uint16_t status_port) :
		m_Enabled(enabled),
		m_StatusPort(status_port)
	{
	}

	HTTP::Message WebRoute_Diagnostics_Matter::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Diagnostics_Matter::OnRequest", std::source_location::current());

		if (req.method() != HTTP::Verbs::get)
		{
			return HTTP::Responses::Response_405(req);
		}

		nlohmann::json json;
		json["enabled"] = m_Enabled;

		if (!m_Enabled)
		{
			return MakeJsonResponse(req, HTTP::Status::ok, json.dump());
		}

		json["status_port"] = m_StatusPort;

		auto status_body = FetchSidecarStatus(m_StatusPort);
		if (!status_body.has_value())
		{
			// Enabled but the sidecar is not answering (still starting, or crashed).
			json["reachable"] = false;
			json["running"] = false;
			json["paired"] = false;
			return MakeJsonResponse(req, HTTP::Status::ok, json.dump());
		}

		json["reachable"] = true;

		// Merge the sidecar's status object ({ running, paired, fabrics, qr_payload,
		// manual_code }) into the response. Tolerate a malformed body defensively.
		if (auto parsed = nlohmann::json::parse(status_body.value(), nullptr, false); !parsed.is_discarded() && parsed.is_object())
		{
			for (auto it = parsed.begin(); it != parsed.end(); ++it)
			{
				json[it.key()] = it.value();
			}
		}

		return MakeJsonResponse(req, HTTP::Status::ok, json.dump());
	}

}
// namespace AqualinkAutomate::HTTP
