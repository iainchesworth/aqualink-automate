#include <filesystem>
#include <format>
#include <system_error>

#include <boost/url/parse.hpp>

#include "http/server/static_file_handler.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

    StaticFileHandler::StaticFileHandler(std::string_view prefix, const std::filesystem::path& doc_root) :
        StaticFileHandler(boost::urls::parse_uri_reference(prefix).value(), doc_root)
    {
    }

    StaticFileHandler::StaticFileHandler(boost::urls::url prefix, const std::filesystem::path& doc_root) :
        m_Prefix(std::move(prefix)),
        m_DocRoot(std::filesystem::absolute(doc_root))
    {
        std::error_code ec;

        if (!std::filesystem::is_directory(m_DocRoot, ec))
        {
            LogWarning(Channel::Web, std::format("Specified static directory '{}' was invalid; error was -> {}", m_DocRoot.string(), ec.message()));
        }
    }

    bool StaticFileHandler::match(const boost::urls::url_view& target, std::filesystem::path& result)
    {
        if (boost::system::result<boost::urls::url> parsed_target = boost::urls::parse_uri_reference(target); parsed_target.has_error())
        {
            LogDebug(Channel::Web, std::format("Failed to parse target URL as a static asset; error was {}", parsed_target.error().message()));
        }
        else if (auto prefix_len = match_prefix(parsed_target.value().normalize_path().segments(), static_cast<boost::urls::url_view>(m_Prefix).segments()); prefix_len >= 0)
        {
            result = m_DocRoot;

            auto segs = target.segments();
            auto it = segs.begin();
            auto end = segs.end();

            // Skip only the non-empty prefix segments that were actually matched
            std::advance(it, prefix_len);

            while (it != end)
            {
                auto seg = *it;
                if (!seg.empty())
                {
                    result.append(seg.begin(), seg.end());
                }
                ++it;
            }

            // If result points to a directory, serve index.html
            if (std::filesystem::is_directory(result))
            {
                result /= "index.html";
            }

            // Security: Verify resolved path is within document root (path jail)
            std::error_code canon_ec;
            auto canonical_result = std::filesystem::weakly_canonical(result, canon_ec);
            auto canonical_root = std::filesystem::weakly_canonical(m_DocRoot, canon_ec);

            if (canon_ec)
            {
                LogWarning(Channel::Web, std::format("Path canonicalization failed: {}", canon_ec.message()));
                return false;
            }

            // Ensure the resolved path starts with the document root
            auto result_str = canonical_result.string();
            auto root_str = canonical_root.string();
            if (result_str.size() < root_str.size() ||
                result_str.compare(0, root_str.size(), root_str) != 0)
            {
                LogWarning(Channel::Web, std::format("Path traversal attempt blocked: {} escapes {}", result_str, root_str));
                return false;
            }

            return true;
        }

        return false;
    }

    int StaticFileHandler::match_prefix(boost::urls::segments_view target, boost::urls::segments_view prefix)
    {
        // Count the non-empty segments in the prefix (root "/" has one empty segment which we skip)
        std::size_t non_empty_prefix_count = 0;
        for (const auto& seg : prefix)
        {
            if (!seg.empty())
            {
                ++non_empty_prefix_count;
            }
        }

        // A root prefix "/" matches everything
        if (non_empty_prefix_count == 0)
        {
            return 0;
        }

        // Trivially reject target that cannot contain the prefix
        auto target_size = target.size();
        if (target_size < non_empty_prefix_count)
        {
            return -1;
        }

        // Match the non-empty prefix segments against target segments
        auto it_target = target.begin();
        auto end_target = target.end();
        auto it_prefix = prefix.begin();
        auto end_prefix = prefix.end();
        int matched = 0;

        while (it_target != end_target && it_prefix != end_prefix)
        {
            // Skip empty segments in both
            if ((*it_prefix).empty())
            {
                ++it_prefix;
                continue;
            }
            if ((*it_target).empty())
            {
                ++it_target;
                ++matched;
                continue;
            }

            if (*it_target != *it_prefix)
            {
                return -1;
            }

            ++it_target;
            ++it_prefix;
            ++matched;
        }

        // Check all non-empty prefix segments were consumed
        while (it_prefix != end_prefix)
        {
            if (!(*it_prefix).empty())
            {
                return -1;
            }
            ++it_prefix;
        }

        return matched;
    }

}
// namespace AqualinkAutomate::HTTP
