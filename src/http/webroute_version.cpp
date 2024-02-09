#include <nlohmann/json.hpp>

#include "http/webroute_version.h"
#include "http/server/server_fields.h"
#include "version/version.h"

namespace AqualinkAutomate::HTTP
{

	boost::beast::http::message_generator WebRoute_Version::OnRequest(const HTTP::Request& req)
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

		HTTP::Response resp{ HTTP::Status::ok, req.version() };

		resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
		resp.keep_alive(req.keep_alive());
		resp.body() = version_info.dump();
		resp.prepare_payload();

		return resp;
	}

}
// namespace AqualinkAutomate::HTTP
