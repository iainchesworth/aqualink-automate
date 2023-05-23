#include <nlohmann/json.hpp>

#include "http/webroute_version.h"
#include "version/version.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Version::WebRoute_Version(crow::SimpleApp& app, const std::string& doc_root) :
		Interfaces::IWebRoute<VERSION_ROUTE_URL>(app, doc_root)
	{
	}

	void WebRoute_Version::WebRequestHandler(const Request& req, Response& resp)
	{	
		nlohmann::json version_info;

		version_info["software_version"]["name"] = Version::VersionInfo::ProjectName();
		version_info["software_version"]["version"] = Version::VersionInfo::ProjectVersion();
		version_info["software_version"]["description"] = Version::VersionInfo::ProjectDescription();
		version_info["software_version"]["homepage"] = Version::VersionInfo::ProjectHomepageURL();

		if (Version::GitMetadata::Populated())
		{
			version_info["git_info"]["commit_sha1"] = Version::GitMetadata::CommitSHA1();
			version_info["git_info"]["commit_date"] = Version::GitMetadata::CommitDate();
			version_info["git_info"]["uncommitted_changes"] = Version::GitMetadata::AnyUncommittedChanges();
		}

		resp.set_header("Content-Type", "application/json");
		resp.body = version_info.dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
