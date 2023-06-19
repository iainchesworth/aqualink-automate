#include "version/version_cmake.h"

namespace AqualinkAutomate::Version
{

	std::string VersionInfo::ProjectName()
	{
		return PROJECT_NAME;
	}

	std::string VersionInfo::ProjectVersion()
	{
		return PROJECT_VERSION;
	}

	std::string VersionInfo::ProjectDescription()
	{
		return PROJECT_DESCRIPTION;
	}

	std::string VersionInfo::ProjectHomepageURL()
	{
		return PROJECT_HOMEPAGE_URL;
	}

	uint32_t VersionInfo::Major()
	{
		return VERSION_MAJOR;
	}

	uint32_t VersionInfo::Minor()
	{
		return VERSION_MINOR;
	}

	uint32_t VersionInfo::Patch()
	{
		return VERSION_PATCH;
	}

}
// namespace AqualinkAutomate::Version
