#pragma once

#include <string>

#include "version/version_cmake.h"
#include "version/version_git.h"

namespace AqualinkAutomate::Version
{

	std::string VersionDetails();
	std::string GitCommitDetails();

}
// namespace AqualinkAutomate::Version
