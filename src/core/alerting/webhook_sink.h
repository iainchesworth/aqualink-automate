#pragma once

#include <functional>
#include <string>

#include <boost/asio/io_context.hpp>

#include "alerting/alert_condition.h"

namespace AqualinkAutomate::Alerting
{

	//=========================================================================
	// WebhookSink — best-effort async HTTP/HTTPS POST of alert transitions.
	//
	// On each transition it POSTs a JSON body
	//   {"condition": "...", "state": "raised"|"cleared", "ts": <unix>, "detail": "..."}
	// to the URL returned by the supplied provider.  The provider is read FRESH
	// on every transition so the webhook URL can be changed at runtime via the
	// preferences API (an empty/invalid URL is simply a no-op).  Runs entirely on
	// the application io_context (no extra threads, no blocking waits), retries
	// once on failure, then logs and drops — an unreachable webhook must NEVER
	// wedge the main loop.
	//=========================================================================
	class WebhookSink
	{
	public:
		using UrlProvider = std::function<std::string()>;

		WebhookSink(boost::asio::io_context& io_context, UrlProvider url_provider);

		// Fire-and-forget POST for one transition (returns immediately). No-op
		// when the current URL is empty or not a valid http/https URL.
		void Post(const AlertTransition& transition);

		// Build the JSON request body for a transition (pure; unit-tested).
		static std::string BuildPayload(const AlertTransition& transition);

	private:
		boost::asio::io_context& m_IoContext;
		UrlProvider m_UrlProvider;
	};

}
// namespace AqualinkAutomate::Alerting
