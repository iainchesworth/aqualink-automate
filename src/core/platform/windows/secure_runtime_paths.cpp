#include "application/secure_runtime_paths.h"

#include <cstdlib>
#include <system_error>

namespace fs = std::filesystem;

namespace
{

	// Value of environment variable `name` if it is set and non-empty, else nullptr.
	const char* NonEmptyEnv(const char* name)
	{
		const char* v = std::getenv(name);
		return (nullptr != v && '\0' != v[0]) ? v : nullptr;
	}

}

namespace AqualinkAutomate::Application
{

	std::vector<fs::path> SecureRuntimeStateDirectories()
	{
		std::vector<fs::path> dirs;

		// %LOCALAPPDATA% (C:\Users\<user>\AppData\Local) is per-user and, by default,
		// ACL-restricted to that user plus SYSTEM/Administrators -- other standard
		// users cannot read it. This is the Windows analogue of a private state dir.
		if (const char* lad = NonEmptyEnv("LOCALAPPDATA"); nullptr != lad)
		{
			dirs.emplace_back(fs::path{ lad } / "aqualink-automate");
		}
		else if (const char* up = NonEmptyEnv("USERPROFILE"); nullptr != up)
		{
			dirs.emplace_back(fs::path{ up } / "AppData" / "Local" / "aqualink-automate");
		}

		return dirs;
	}

	bool PrepareSecureDirectory(const fs::path& dir)
	{
		if (dir.empty())
		{
			return false;
		}

		std::error_code ec;
		fs::create_directories(dir, ec);

		// symlink_status does not follow the final component, so a symlink / reparse
		// point planted at `dir` is detected rather than followed.
		const fs::file_status st = fs::symlink_status(dir, ec);
		if (ec || fs::is_symlink(st) || !fs::is_directory(st))
		{
			return false;
		}

		// Windows access control is ACL-based rather than POSIX owner bits; we rely on
		// the per-user ACL inherited from %LOCALAPPDATA% rather than a chmod. Rejecting
		// symlinks/reparse points above blocks the redirect-to-shared-location vector.
		return true;
	}

}
// namespace AqualinkAutomate::Application
