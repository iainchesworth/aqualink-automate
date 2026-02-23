#include <format>
#include <string>

#include "http/server/server_fields.h"
#include "version/version.h"

namespace AqualinkAutomate::HTTP
{

	const std::string_view ServerFields::RetryAfter()
	{
		static const std::string_view DEFAULT_RETRY_AFTER{ "30" };
		return DEFAULT_RETRY_AFTER;
	}

	const std::string_view ServerFields::Server()
	{
		static const std::string SERVER_VERSION_STRING
		{
			std::format("{}/{}", Version::VersionInfo::ProjectName(), Version::VersionInfo::ProjectVersion())
		};

		return std::string_view(SERVER_VERSION_STRING);
	}

}
// namespace AqualinkAutomate::HTTP
