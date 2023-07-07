#include <format>
#include <string>

#include "http/support/support_generate_page_footer.h"
#include "version/version.h"

namespace AqualinkAutomate::HTTP::Support
{

	void GeneratePageFooter_Context(std::unordered_map<std::string, std::string>& template_value_map)
	{
		auto generate_git_string = []() -> std::string
		{
			auto git_details = std::string("No git metadata available.");

			if (Version::GitMetadata::Populated())
			{
				git_details = std::format
				(
					"Git Commit SHA1: {} | Uncommitted Changes: {}",
					Version::GitMetadata::CommitSHA1().substr(0, 6),  // Short GIT SHA1
					(Version::GitMetadata::AnyUncommittedChanges() ? "Yes" : "No")
				);
			}

			return git_details;
		};

		template_value_map.emplace("aqualink_automate_version", Version::VersionInfo::ProjectVersion());
		template_value_map.emplace("aqualink_automate_gitinfo", generate_git_string());
		template_value_map.emplace("aqualink_automate_github", Version::VersionInfo::ProjectHomepageURL());
	}

}
// namespace AqualinkAutomate::HTTP::Support
