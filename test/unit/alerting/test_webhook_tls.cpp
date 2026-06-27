#include <boost/test/unit_test.hpp>

#include <boost/asio/ssl/context.hpp>
#include <openssl/ssl.h>

#include "alerting/webhook_tls.h"

using namespace AqualinkAutomate::Alerting;

//=============================================================================
// Outbound alert-webhook TLS verification.
//
// Regression lock for the MITM gap where AttemptHttps() built a tls_client
// context and loaded the trust store but never enabled peer verification, so a
// freshly-constructed Boost.Asio context stayed at verify_none and the handshake
// would succeed against ANY server certificate.  ApplyClientTlsVerification must
// switch the context to verify_peer so an https:// webhook is authenticated, not
// merely encrypted.
//=============================================================================

BOOST_AUTO_TEST_SUITE(WebhookTls_TestSuite)

BOOST_AUTO_TEST_CASE(FreshClientContext_DefaultsToNoVerification)
{
	// Documents the dangerous default this fix guards against.
	boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_client);
	BOOST_CHECK_EQUAL(SSL_CTX_get_verify_mode(ctx.native_handle()), SSL_VERIFY_NONE);
}

BOOST_AUTO_TEST_CASE(ApplyClientTlsVerification_RequiresPeerVerification)
{
	boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_client);

	ApplyClientTlsVerification(ctx);

	const int mode = SSL_CTX_get_verify_mode(ctx.native_handle());
	BOOST_CHECK((mode & SSL_VERIFY_PEER) == SSL_VERIFY_PEER);
}

BOOST_AUTO_TEST_SUITE_END()
