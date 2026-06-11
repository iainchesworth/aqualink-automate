#include "application/application_defaults.h"

#include <filesystem>
#include <string>

#include <boost/dll/runtime_symbol_info.hpp>

namespace
{

	// Resolve a bundled asset relative to the installed executable. The Windows
	// package keeps the web/ssl assets beside the executable in bin/ (see
	// cmake/CPackConfig.cmake), so the asset path is exe_dir/<relative>.
	std::string AssetPath(const std::string& relative)
	{
		try
		{
			const std::filesystem::path exe{ boost::dll::program_location().string() };
			return (exe.parent_path() / relative).lexically_normal().string();
		}
		catch (const std::exception&)
		{
			// program_location can throw if the executable path is unavailable;
			// fall back to a working-directory-relative path.
			return std::filesystem::path(relative).lexically_normal().string();
		}
	}

}

namespace AqualinkAutomate::Application
{

	const std::string SERIAL_PORT{ "COM1" };
	const std::string DOC_ROOT{ AssetPath("web") };
	const uint16_t DEFAULT_HTTP_PORT{ 80 };
	const uint16_t DEFAULT_HTTPS_PORT{ 443 };
	const std::string DEFAULT_CERTIFICATE{ AssetPath("ssl/cert.pem") };
	const std::string DEFAULT_PRIVATE_KEY{ AssetPath("ssl/key.pem") };

}
// namespace AqualinkAutomate::Application
