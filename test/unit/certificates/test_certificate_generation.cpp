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

//=============================================================================
// Read-only-install fallback (Detail::GenerateOrReuseInDirectories).
//
// This is the security-critical branch that backs EnsureSelfSignedMaterial when
// the built-in default cert/key paths are not writable (the common packaged
// case): it walks candidate directories in order and, in the first writable
// one, reuses or freshly generates a per-install self-signed pair. Testing the
// helper directly lets us drive that path with controlled directories instead
// of depending on the real (install-tree) default paths being writable.
//=============================================================================

BOOST_AUTO_TEST_CASE(Fallback_GeneratesFreshPairInFirstWritableDirectory)
{
	const fs::path base = fs::temp_directory_path() / "aqualink_certgen_fallback_gen";
	std::error_code rm;
	fs::remove_all(base, rm);

	const fs::path secure_dir = base / "ssl";

	const auto resolved = Certificates::Detail::GenerateOrReuseInDirectories({ secure_dir });
	BOOST_REQUIRE(resolved.has_value());

	// Material must land in the chosen secure directory under the fixed names.
	BOOST_CHECK(resolved->certificate == secure_dir / "cert.pem");
	BOOST_CHECK(resolved->private_key == secure_dir / "key.pem");
	BOOST_REQUIRE(fs::exists(resolved->certificate));
	BOOST_REQUIRE(fs::exists(resolved->private_key));

	// The freshly generated material must be loadable by the TLS stack.
	boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_server);
	boost::system::error_code ec;
	ctx.use_certificate_file(resolved->certificate.string(), boost::asio::ssl::context::pem, ec);
	BOOST_CHECK_MESSAGE(!ec, "generated certificate did not load: " << ec.message());
	ctx.use_private_key_file(resolved->private_key.string(), boost::asio::ssl::context::pem, ec);
	BOOST_CHECK_MESSAGE(!ec, "generated private key did not load: " << ec.message());

#ifndef _WIN32
	// The private key must be owner-only (0600) on disk.
	const auto perms = fs::status(resolved->private_key).permissions();
	BOOST_CHECK((perms & fs::perms::group_read) == fs::perms::none);
	BOOST_CHECK((perms & fs::perms::others_read) == fs::perms::none);
	BOOST_CHECK((perms & fs::perms::others_write) == fs::perms::none);
#endif

	fs::remove_all(base, rm);
}

BOOST_AUTO_TEST_CASE(Fallback_ReusesExistingPairAcrossCalls)
{
	const fs::path base = fs::temp_directory_path() / "aqualink_certgen_fallback_reuse";
	std::error_code rm;
	fs::remove_all(base, rm);

	const fs::path secure_dir = base / "ssl";

	// First call generates the pair.
	const auto first = Certificates::Detail::GenerateOrReuseInDirectories({ secure_dir });
	BOOST_REQUIRE(first.has_value());
	const std::string key_after_first = ReadFile(first->private_key);
	const std::string cert_after_first = ReadFile(first->certificate);
	BOOST_REQUIRE(!key_after_first.empty());

	// Second call must REUSE the existing pair -- same paths, byte-identical
	// material (i.e. it must not regenerate and clobber the persisted key).
	const auto second = Certificates::Detail::GenerateOrReuseInDirectories({ secure_dir });
	BOOST_REQUIRE(second.has_value());
	BOOST_CHECK(second->certificate == first->certificate);
	BOOST_CHECK(second->private_key == first->private_key);
	BOOST_CHECK(ReadFile(second->private_key) == key_after_first);
	BOOST_CHECK(ReadFile(second->certificate) == cert_after_first);

	fs::remove_all(base, rm);
}

BOOST_AUTO_TEST_CASE(Fallback_SkipsUnwritableCandidateAndUsesNext)
{
	const fs::path base = fs::temp_directory_path() / "aqualink_certgen_fallback_skip";
	std::error_code rm;
	fs::remove_all(base, rm);
	fs::create_directories(base, rm);

	// A candidate that cannot be used as a directory: a path that already exists
	// as a regular file. create_directories() fails on it, so the fallback must
	// skip it and move on to the next candidate. (Cross-platform: no reliance on
	// POSIX-only read-only-directory semantics.)
	const fs::path blocked_candidate = base / "not-a-directory";
	{
		std::ofstream f(blocked_candidate);
		f << "occupied";
	}
	BOOST_REQUIRE(fs::is_regular_file(blocked_candidate));

	const fs::path writable_dir = base / "ssl";

	const auto resolved = Certificates::Detail::GenerateOrReuseInDirectories({ blocked_candidate, writable_dir });
	BOOST_REQUIRE(resolved.has_value());

	// It must have fallen through to the second (writable) candidate.
	BOOST_CHECK(resolved->certificate == writable_dir / "cert.pem");
	BOOST_CHECK(resolved->private_key == writable_dir / "key.pem");
	BOOST_CHECK(fs::exists(resolved->certificate));
	BOOST_CHECK(fs::exists(resolved->private_key));

	fs::remove_all(base, rm);
}

BOOST_AUTO_TEST_CASE(Fallback_ReturnsNulloptWhenNoCandidateIsWritable)
{
	const fs::path base = fs::temp_directory_path() / "aqualink_certgen_fallback_none";
	std::error_code rm;
	fs::remove_all(base, rm);
	fs::create_directories(base, rm);

	// Every candidate is an existing regular file, so none can be created/written
	// as a directory. With no usable candidate the fallback yields nullopt (the
	// caller then surfaces the "could not be generated" error).
	const fs::path blocked_a = base / "blocked-a";
	const fs::path blocked_b = base / "blocked-b";
	for (const auto& p : { blocked_a, blocked_b })
	{
		std::ofstream f(p);
		f << "occupied";
	}

	// An empty path is also not writable and must be tolerated/skipped.
	const auto resolved = Certificates::Detail::GenerateOrReuseInDirectories({ fs::path{}, blocked_a, blocked_b });
	BOOST_CHECK(!resolved.has_value());

	fs::remove_all(base, rm);
}

BOOST_AUTO_TEST_SUITE_END()
