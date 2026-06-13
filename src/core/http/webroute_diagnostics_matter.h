#pragma once

#include <cstdint>

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char DIAGNOSTICS_MATTER_ROUTE_URL[] = "/api/diagnostics/matter";

	// Live Matter-bridge status for the diagnostics page. The Matter stack runs in a
	// separate sidecar process (matter-bridge/), so this route proxies the sidecar's
	// status/QR endpoint (http://127.0.0.1:<status_port>/matter/status) and merges in
	// the "enabled" flag from the Matter settings. Returns
	//   { "enabled": false }
	// when Matter is opted out, and
	//   { "enabled": true, "reachable": false }
	// when enabled but the sidecar is not (yet) answering.
	class WebRoute_Diagnostics_Matter : public Interfaces::IWebRoute<DIAGNOSTICS_MATTER_ROUTE_URL>
	{
	public:
		WebRoute_Diagnostics_Matter(bool enabled, uint16_t status_port);
		~WebRoute_Diagnostics_Matter() override = default;

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		const bool m_Enabled;
		const uint16_t m_StatusPort;
	};

}
// namespace AqualinkAutomate::HTTP
