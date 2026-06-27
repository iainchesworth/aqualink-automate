#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/asio/ssl/context.hpp>
#include <boost/system/error_code.hpp>

#include "application/application_defaults.h"
#include "certificates/certificate_management.h"

namespace fs = std::filesystem;
using namespace AqualinkAutomate;

//=============================================================================
// Per-install self-signed certificate generation.
//
// Replaces the historical, repository-committed shared private key (identical
// on every install). The security-critical property is UNIQUENESS: two installs
// (here, two generations) must NOT share a key. Also lock that the generated
// material is loadable by the TLS stack and that the private key is owner-only.
//=============================================================================

namespace
{
	std::string ReadFile(const fs::path& p)
	{
		std::ifstream in(p, std::ios::binary);
		std::ostringstream ss;
		ss << in.rdbuf();
		return ss.str();
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_CertificateGeneration)

BOOST_AUTO_TEST_CASE(GenerateSelfSigned_ProducesParseableUniqueMaterial)
{
	const fs::path dir = fs::temp_directory_path() / "aqualink_certgen_test";
	std::error_code rm;
	fs::remove_all(dir, rm);

	const fs::path cert = dir / "cert.pem";
	const fs::path key = dir / "key.pem";

	BOOST_REQUIRE(Certificates::GenerateSelfSignedCertificate(cert, key));
	BOOST_REQUIRE(fs::exists(cert));
	BOOST_REQUIRE(fs::exists(key));

	// The generated material must be loadable by the TLS stack.
	boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_server);
	boost::system::error_code ec;
	ctx.use_certificate_file(cert.string(), boost::asio::ssl::context::pem, ec);
	BOOST_CHECK_MESSAGE(!ec, "certificate did not load: " << ec.message());
	ctx.use_private_key_file(key.string(), boost::asio::ssl::context::pem, ec);
	BOOST_CHECK_MESSAGE(!ec, "private key did not load: " << ec.message());

	// Uniqueness: a second generation must produce a DIFFERENT private key.
	const fs::path cert2 = dir / "cert2.pem";
	const fs::path key2 = dir / "key2.pem";
	BOOST_REQUIRE(Certificates::GenerateSelfSignedCertificate(cert2, key2));
	BOOST_CHECK(ReadFile(key) != ReadFile(key2));
	BOOST_CHECK(ReadFile(cert) != ReadFile(cert2));

#ifndef _WIN32
	// The private key must not be group/world readable (0600).
	const auto perms = fs::status(key).permissions();
	BOOST_CHECK((perms & fs::perms::group_read) == fs::perms::none);
	BOOST_CHECK((perms & fs::perms::others_read) == fs::perms::none);
	BOOST_CHECK((perms & fs::perms::others_write) == fs::perms::none);
#endif

	fs::remove_all(dir, rm);
}

BOOST_AUTO_TEST_CASE(EnsureSelfSignedMaterial_UsesExistingFiles)
{
	const fs::path dir = fs::temp_directory_path() / "aqualink_certgen_existing";
	std::error_code rm;
	fs::remove_all(dir, rm);

	const fs::path cert = dir / "cert.pem";
	const fs::path key = dir / "key.pem";
	BOOST_REQUIRE(Certificates::GenerateSelfSignedCertificate(cert, key));

	const auto resolved = Certificates::EnsureSelfSignedMaterial(Options::Web::SslCertificate{ cert, key });
	BOOST_REQUIRE(resolved.has_value());
	BOOST_CHECK(resolved->certificate == cert);
	BOOST_CHECK(resolved->private_key == key);

	fs::remove_all(dir, rm);
}

BOOST_AUTO_TEST_CASE(EnsureSelfSignedMaterial_RefusesToGenerateForOperatorSpecifiedMissingPaths)
{
	// An operator who explicitly points --cert/--cert-key at a non-default path
	// that does not exist must NOT have a cert silently fabricated for them: that
	// is a misconfiguration the caller surfaces as an error.
	const fs::path missing_dir = fs::temp_directory_path() / "aqualink_certgen_missing";
	std::error_code rm;
	fs::remove_all(missing_dir, rm);

	Options::Web::SslCertificate operator_specified{ missing_dir / "their-cert.pem", missing_dir / "their-key.pem" };
	const auto resolved = Certificates::EnsureSelfSignedMaterial(operator_specified);
	BOOST_CHECK(!resolved.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
