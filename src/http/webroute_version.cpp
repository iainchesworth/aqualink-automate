#include <nlohmann/json.hpp>

#include "http/webroute_version.h"
#include "version/version.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Version::WebRoute_Version(HTTP::Server& http_server) :
		Interfaces::IWebRoute<VERSION_ROUTE_URL>(http_server, { { HTTP::Methods::GET, std::bind(&WebRoute_Version::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		Interfaces::IShareableRoute()
	{
	}

	void WebRoute_Version::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
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

		resp.set_status_and_content(
			cinatra::status_type::ok,
			version_info.dump(),
			cinatra::req_content_type::json,
			cinatra::content_encoding::none
		);
	}

}
// namespace AqualinkAutomate::HTTP
