#include <chrono>
#include <exception>
#include <format>
#include <utility>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>

#include <nlohmann/json.hpp>

#include "alerting/webhook_sink.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace AqualinkAutomate::Alerting
{
	namespace
	{
		constexpr std::chrono::seconds WEBHOOK_TIMEOUT{ 10 };

		http::request<http::string_body> BuildRequest(const std::string& host, const std::string& target, const std::string& body)
		{
			http::request<http::string_body> req{ http::verb::post, target.empty() ? "/" : target, 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, "aqualink-automate");
			req.set(http::field::content_type, "application/json");
			req.body() = body;
			req.prepare_payload();
			return req;
		}

		// One HTTP attempt over a plain TCP stream.  Throws on any failure.
		asio::awaitable<void> AttemptHttp(std::string host, std::string port, std::string target, std::string body)
		{
			auto executor = co_await asio::this_coro::executor;
			tcp::resolver resolver(executor);
			beast::tcp_stream stream(executor);

			auto results = co_await resolver.async_resolve(host, port, asio::use_awaitable);
			stream.expires_after(WEBHOOK_TIMEOUT);
			co_await stream.async_connect(results, asio::use_awaitable);

			auto req = BuildRequest(host, target, body);
			stream.expires_after(WEBHOOK_TIMEOUT);
			co_await http::async_write(stream, req, asio::use_awaitable);

			beast::flat_buffer buffer;
			http::response<http::string_body> res;
			co_await http::async_read(stream, buffer, res, asio::use_awaitable);

			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);
		}

		// One HTTPS attempt over a TLS stream.  Throws on any failure.
		asio::awaitable<void> AttemptHttps(std::string host, std::string port, std::string target, std::string body)
		{
			auto executor = co_await asio::this_coro::executor;
			tcp::resolver resolver(executor);

			ssl::context ctx(ssl::context::tls_client);
			ctx.set_default_verify_paths();

			beast::ssl_stream<beast::tcp_stream> stream(executor, ctx);

			// SNI is mandatory for most modern TLS servers.
			if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
			{
				throw beast::system_error{ beast::error_code{ static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category() } };
			}

			auto results = co_await resolver.async_resolve(host, port, asio::use_awaitable);
			beast::get_lowest_layer(stream).expires_after(WEBHOOK_TIMEOUT);
			co_await beast::get_lowest_layer(stream).async_connect(results, asio::use_awaitable);

			co_await stream.async_handshake(ssl::stream_base::client, asio::use_awaitable);

			auto req = BuildRequest(host, target, body);
			beast::get_lowest_layer(stream).expires_after(WEBHOOK_TIMEOUT);
			co_await http::async_write(stream, req, asio::use_awaitable);

			beast::flat_buffer buffer;
			http::response<http::string_body> res;
			co_await http::async_read(stream, buffer, res, asio::use_awaitable);

			beast::error_code ec;
			co_await stream.async_shutdown(asio::redirect_error(asio::use_awaitable, ec));
		}

		// Run the POST with a single retry; on final failure, log and drop so the
		// main loop is never affected by an unreachable webhook.
		asio::awaitable<void> RunWithRetry(bool tls, std::string host, std::string port, std::string target, std::string body)
		{
			for (int attempt = 1; attempt <= 2; ++attempt)
			{
				try
				{
					if (tls)
					{
						co_await AttemptHttps(host, port, target, body);
					}
					else
					{
						co_await AttemptHttp(host, port, target, body);
					}
					co_return;
				}
				catch (const std::exception& ex)
				{
					if (attempt == 2)
					{
						LogWarning(Channel::Web, [host, ex_what = std::string{ ex.what() }] { return std::format("Alert webhook POST to {} failed (dropped): {}", host, ex_what); });
					}
				}
			}
			co_return;
		}
	}
	// unnamed namespace

	WebhookSink::WebhookSink(boost::asio::io_context& io_context, std::string url) :
		m_IoContext(io_context),
		m_Url(std::move(url))
	{
		auto parsed = boost::urls::parse_uri(m_Url);
		if (!parsed)
		{
			return;
		}

		const auto scheme = parsed->scheme();
		if (scheme != "http" && scheme != "https")
		{
			return;
		}

		m_UseTls = (scheme == "https");
		m_Host = parsed->host();
		m_Port = parsed->has_port() ? std::string{ parsed->port() } : (m_UseTls ? "443" : "80");

		// Path + (optional) query reassembled as the request target.
		std::string target{ parsed->path() };
		if (target.empty())
		{
			target = "/";
		}
		if (parsed->has_query())
		{
			target += "?";
			target += parsed->query();
		}
		m_Target = std::move(target);

		m_Valid = !m_Host.empty();
	}

	std::string WebhookSink::BuildPayload(const AlertTransition& transition)
	{
		nlohmann::json payload;
		payload["condition"] = transition.condition;
		payload["state"] = transition.raised ? "raised" : "cleared";
		payload["ts"] = transition.ts;
		payload["detail"] = transition.detail;
		return payload.dump();
	}

	void WebhookSink::Post(const AlertTransition& transition)
	{
		if (!m_Valid)
		{
			return;
		}

		boost::asio::co_spawn(
			m_IoContext,
			RunWithRetry(m_UseTls, m_Host, m_Port, m_Target, BuildPayload(transition)),
			boost::asio::detached);
	}

}
// namespace AqualinkAutomate::Alerting
