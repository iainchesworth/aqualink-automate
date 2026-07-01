#pragma once

#include <filesystem>
#include <vector>

namespace AqualinkAutomate::Application
{

	// Ordered, per-user PRIVATE directories in which generated TLS material may be
	// stored when the configured (install) directory is read-only. Every entry is,
	// by construction, owned by and accessible only to the current user -- NEVER a
	// world-writable location such as the system temp directory. Each returned path
	// is already namespaced to the application; the caller appends its own leaf
	// (e.g. "ssl"). The list may legitimately be empty (e.g. a daemon with no state,
	// runtime, or home directory), in which case no fallback is attempted.
	//
	// Resolution order:
	//   POSIX   : $STATE_DIRECTORY (systemd StateDirectory=, persistent, owned by the
	//             service user) -> $XDG_RUNTIME_DIR (user session, tmpfs, 0700) ->
	//             $HOME/.local/state.
	//   Windows : %LOCALAPPDATA% (per-user, ACL-restricted) -> %USERPROFILE%\AppData\Local.
	std::vector<std::filesystem::path> SecureRuntimeStateDirectories();

	// Create `dir` and any missing parents, restrict it to the current user only
	// (0700 on POSIX; the inherited per-user ACL on Windows), and verify the result
	// is a real directory that is NOT a symlink / reparse point and is owned by the
	// current user. Returns true ONLY when the directory is safe to store secrets in.
	// An existing directory owned by another user, or that is a symlink, is rejected
	// -- this defeats pre-creation / symlink attacks on a shared path.
	bool PrepareSecureDirectory(const std::filesystem::path& dir);

}
// namespace AqualinkAutomate::Application
