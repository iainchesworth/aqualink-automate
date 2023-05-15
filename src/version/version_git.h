#pragma once

#include <string>

namespace AqualinkAutomate::Version
{

	class GitMetadata
	{
		static const bool GIT_RETRIEVED_STATE;
		static const bool GIT_IS_DIRTY;
		static const std::string GIT_AUTHOR_NAME;
		static const std::string GIT_AUTHOR_EMAIL;
		static const std::string GIT_HEAD_SHA1;
		static const std::string GIT_COMMIT_DATE_ISO8601;
		static const std::string GIT_COMMIT_SUBJECT;
		static const std::string GIT_COMMIT_BODY;
		static const std::string GIT_DESCRIBE;

	public:
		// Is the metadata populated? We may not have metadata if
		// there wasn't a .git directory (e.g. downloaded source
		// code without revision history).
		static bool Populated();

		// Were there any uncommitted changes that won't be reflected
		// in the CommitID?
		static bool AnyUncommittedChanges();

		// The commit author's name.
		static std::string AuthorName();
		// The commit author's email.
		static std::string AuthorEmail();
		// The commit SHA1.
		static std::string CommitSHA1();
		// The ISO8601 commit date.
		static std::string CommitDate();
		// The commit subject.
		static std::string CommitSubject();
		// The commit body.
		static std::string CommitBody();
		// The commit describe.
		static std::string Describe();
	};

}
// namespace AqualinkAutomate::Version
