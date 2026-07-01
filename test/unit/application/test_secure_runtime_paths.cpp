#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <system_error>

#include "application/secure_runtime_paths.h"

namespace fs = std::filesystem;
using namespace AqualinkAutomate;

//=============================================================================
// Secure per-user runtime directories for generated TLS material.
//
// The security-critical property (SonarCloud S5443): the fallback location for
// an auto-generated private key must NEVER be a world-writable directory such
// as the system temp dir. PrepareSecureDirectory must additionally refuse a
// symlinked or foreign-owned path so it cannot be redirected/pre-seeded.
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_SecureRuntimePaths)

BOOST_AUTO_TEST_CASE(RuntimeStateDirectories_NeverUnderTheWorldWritableTempDir)
{
	std::error_code ec;
	const fs::path tmp = fs::temp_directory_path(ec);

	for (const fs::path& dir : Application::SecureRuntimeStateDirectories())
	{
		// No candidate may be the temp dir itself or live beneath it.
		BOOST_CHECK(dir != tmp);
		const auto rel = fs::relative(dir, tmp, ec);
		const bool under_tmp = !ec && !rel.empty() && *rel.begin() != "..";
		BOOST_CHECK_MESSAGE(!under_tmp, "candidate must not be under the temp dir: " << dir.string());
	}
}

BOOST_AUTO_TEST_CASE(PrepareSecureDirectory_CreatesUsableSelfOwnedDirectory)
{
	const fs::path dir = fs::temp_directory_path() / "aqualink_secure_prep_test" / "ssl";
	std::error_code rm;
	fs::remove_all(dir.parent_path(), rm);

	BOOST_REQUIRE(Application::PrepareSecureDirectory(dir));
	BOOST_CHECK(fs::exists(dir));
	BOOST_CHECK(fs::is_directory(dir));

#ifndef _WIN32
	// The directory must be owner-only (0700): no group/other access.
	const auto perms = fs::status(dir).permissions();
	BOOST_CHECK((perms & fs::perms::group_all) == fs::perms::none);
	BOOST_CHECK((perms & fs::perms::others_all) == fs::perms::none);
#endif

	// Idempotent: a second call on the now-existing, self-owned dir still succeeds.
	BOOST_CHECK(Application::PrepareSecureDirectory(dir));

	fs::remove_all(dir.parent_path(), rm);
}

BOOST_AUTO_TEST_CASE(PrepareSecureDirectory_RejectsEmptyPath)
{
	BOOST_CHECK(!Application::PrepareSecureDirectory(fs::path{}));
}

#ifndef _WIN32
BOOST_AUTO_TEST_CASE(PrepareSecureDirectory_RejectsSymlink)
{
	// A symlink planted where the secure dir is expected must be refused rather
	// than followed to its (attacker-chosen) target.
	const fs::path root = fs::temp_directory_path() / "aqualink_secure_symlink_test";
	std::error_code rm;
	fs::remove_all(root, rm);
	fs::create_directories(root, rm);

	const fs::path real_target = root / "real";
	const fs::path link = root / "link";
	fs::create_directories(real_target, rm);

	std::error_code ln_ec;
	fs::create_directory_symlink(real_target, link, ln_ec);
	BOOST_REQUIRE_MESSAGE(!ln_ec, "could not create test symlink: " << ln_ec.message());

	BOOST_CHECK(!Application::PrepareSecureDirectory(link));

	fs::remove_all(root, rm);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
