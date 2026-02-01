#include <format>
#include <system_error>

#include <boost/url/parse.hpp>

#include "http/server/static_file_handler.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

    StaticFileHandler::StaticFileHandler(std::string_view prefix, std::filesystem::path doc_root) :
        StaticFileHandler(boost::urls::parse_uri_reference(prefix).value(), std::move(doc_root))
    {
    }

    StaticFileHandler::StaticFileHandler(boost::urls::url prefix, std::filesystem::path doc_root) :
        m_Prefix(std::move(prefix)),
        m_DocRoot(std::filesystem::absolute(doc_root))
    {
        std::error_code ec;

        if (!std::filesystem::is_directory(m_DocRoot, ec))
        {
            LogWarning(Channel::Web, std::format("Specified static directory '{}' was invalid; error was -> {}", m_DocRoot.string(), ec.message()));
        }
    }

    bool StaticFileHandler::match(boost::urls::url_view target, std::filesystem::path& result)
    {
        if (boost::system::result<boost::urls::url> parsed_target = boost::urls::parse_uri_reference(target); parsed_target.has_error())
        {
            LogDebug(Channel::Web, std::format("Failed to parse target URL as a static asset; error was {}", parsed_target.error().message()));
        }
        else if (match_prefix(parsed_target.value().normalize_path().segments(), static_cast<boost::urls::url_view>(m_Prefix).segments()))
        {
            result = m_DocRoot;

            auto segs = target.segments();
            auto it = segs.begin();
            auto end = segs.end();

            std::advance(it, m_Prefix.segments().size());

            while (it != end)
            {
                auto seg = *it;
                result.append(seg.begin(), seg.end());
                ++it;
            }

            return true;
        }

        return false;
    }

    bool StaticFileHandler::match_prefix(boost::urls::segments_view target, boost::urls::segments_view prefix)
    {
        // Trivially reject target that cannot contain the prefix
        if (target.size() < prefix.size())
        {
            return false;
        }

        // Match the prefix segments
        auto it0 = target.begin();
        auto end0 = target.end();
        auto it1 = prefix.begin();
        auto end1 = prefix.end();

        while (it0 != end0 && it1 != end1 && *it0 == *it1)
        {
            ++it0;
            ++it1;
        }

        return it1 == end1;
    }

}
// namespace AqualinkAutomate::HTTP
