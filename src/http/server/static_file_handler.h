#pragma once

#include <filesystem>
#include <string_view>

#include <boost/url/segments_view.hpp>
#include <boost/url/urls.hpp>
#include <boost/url/url_view.hpp>

namespace AqualinkAutomate::HTTP
{

	class StaticFileHandler
	{
	public:
		StaticFileHandler(std::string_view prefix, std::filesystem::path root);
		StaticFileHandler(boost::urls::url prefix, std::filesystem::path root);

	public:
		bool match(boost::urls::url_view target, std::filesystem::path& result);

	private:
		bool match_prefix(boost::urls::segments_view target, boost::urls::segments_view prefix);

	private:
		boost::urls::url m_Prefix;
		std::filesystem::path m_DocRoot;
	};

}
// namespace AqualinkAutomate::HTTP
