#pragma once

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>

namespace AqualinkAutomate::Alerting
{

	// Configure a TLS *client* context to AUTHENTICATE the server: load the system
	// trust store and require peer verification.
	//
	// Centralised and unit-tested so the outbound-webhook HTTPS path can never
	// silently regress to verify_none.  A freshly constructed Boost.Asio
	// ssl::context defaults to verify_none, which would complete the handshake
	// against ANY certificate (self-signed / expired / wrong-host) -- making an
	// "https://" webhook encrypted but UNauthenticated and trivially MITM-able by
	// an on-path attacker.  The per-stream hostname check (ssl::host_name_verification)
	// is applied at the stream, not here, since it needs the connection's host.
	inline void ApplyClientTlsVerification(boost::asio::ssl::context& ctx)
	{
		ctx.set_default_verify_paths();
		ctx.set_verify_mode(boost::asio::ssl::verify_peer);
	}

}
// namespace AqualinkAutomate::Alerting
