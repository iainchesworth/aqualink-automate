#pragma once

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
	// to the configured URL.  Runs entirely on the application io_context (no
	// extra threads, no blocking waits), retries once on failure, then logs and
	// drops — an unreachable webhook must NEVER wedge the main loop.
	//=========================================================================
	class WebhookSink
	{
	public:
		WebhookSink(boost::asio::io_context& io_context, std::string url);

		// Fire-and-forget POST for one transition (returns immediately).
		void Post(const AlertTransition& transition);

		// Build the JSON request body for a transition (pure; unit-tested).
		static std::string BuildPayload(const AlertTransition& transition);

		// True if the configured URL parsed into a usable http/https target.
		bool IsUsable() const { return m_Valid; }

	private:
		boost::asio::io_context& m_IoContext;
		std::string m_Url;

		// Parsed components.
		bool m_Valid{ false };
		bool m_UseTls{ false };
		std::string m_Host;
		std::string m_Port;
		std::string m_Target;
	};

}
// namespace AqualinkAutomate::Alerting
