#include <filesystem>
#include <string>

#include <boost/test/unit_test.hpp>

#include "application/application_defaults.h"

using namespace AqualinkAutomate;

// =====================================================
// Application asset-path defaults
//
// The default web-root and SSL paths are resolved RELATIVE TO THE EXECUTABLE
// (via boost::dll), not the current working directory, so a packaged install
// self-locates its assets regardless of where it is launched (e.g. as a
// systemd service). These structural assertions guard against a regression to
// the old CWD-relative literals (e.g. "templates/", "ssl/cert.pem"), which
// would make the leaf names / absoluteness fail below.
//
// They deliberately assert STRUCTURE (leaf + parent names, absoluteness) rather
// than a concrete prefix, so they hold on every platform and for any install
// location without hard-coding the build/runner path.
// =====================================================

BOOST_AUTO_TEST_SUITE(TestSuite_ApplicationDefaults)

BOOST_AUTO_TEST_CASE(Test_DocRoot_IsExecutableRelative_WebLeaf)
{
	const std::filesystem::path doc_root{ Application::DOC_ROOT };

	BOOST_CHECK(doc_root.is_absolute());
	BOOST_CHECK_EQUAL(doc_root.filename().string(), "web");
}

BOOST_AUTO_TEST_CASE(Test_DefaultCertificate_IsExecutableRelative_SslCertLeaf)
{
	const std::filesystem::path cert{ Application::DEFAULT_CERTIFICATE };

	BOOST_CHECK(cert.is_absolute());
	BOOST_CHECK_EQUAL(cert.filename().string(), "cert.pem");
	BOOST_CHECK_EQUAL(cert.parent_path().filename().string(), "ssl");
}

BOOST_AUTO_TEST_CASE(Test_DefaultPrivateKey_IsExecutableRelative_SslKeyLeaf)
{
	const std::filesystem::path key{ Application::DEFAULT_PRIVATE_KEY };

	BOOST_CHECK(key.is_absolute());
	BOOST_CHECK_EQUAL(key.filename().string(), "key.pem");
	BOOST_CHECK_EQUAL(key.parent_path().filename().string(), "ssl");
}

BOOST_AUTO_TEST_SUITE_END()
