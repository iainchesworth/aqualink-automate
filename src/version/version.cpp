#include <format>

#include "version/version.h"

namespace AqualinkAutomate::Version
{

	std::string VersionDetails()
	{
		return std::format(
			"Name: {}\nVersion: v{}\nDescription: {}\nHomepage URL: {}",
			Version::VersionInfo::ProjectName(),
			Version::VersionInfo::ProjectVersion(),
			Version::VersionInfo::ProjectDescription(),
			Version::VersionInfo::ProjectHomepageURL()
		);
	}

	std::string GitCommitDetails()
	{
		auto git_details = std::string("No git metadata available.");

		if (Version::GitMetadata::Populated())
		{
			git_details = std::format
			(
				"\nGit Details:\nCommit SHA1: {}\nCommit Date: {}\nUncommitted Changes: {}",
				Version::GitMetadata::CommitSHA1(),
				Version::GitMetadata::CommitDate(),
				(Version::GitMetadata::AnyUncommittedChanges() ? "Yes" : "No")
			);
		}

		return git_details;
	}

}
// namespace AqualinkAutomate::Version
