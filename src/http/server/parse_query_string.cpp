#include <boost/url.hpp>

#include "http/server/parse_query_string.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

    tl::expected<std::string, bool> ParseQueryString(const Request& req, const std::string& query_parameter)
    {
        if (auto url = boost::urls::parse_origin_form(req.target()); url.has_error())
        {
            LogDebug(Channel::Web, std::format("URL was malformed and could not be parsed; error was -> {}", url.error().message()));
        }
        else if (auto pv = url->params(); !pv.contains(query_parameter, boost::urls::ignore_case))
        {
            LogDebug(Channel::Web, "URL was malformed and was missing expected query parameters");
        }
        else if (auto pv_it = pv.find(query_parameter, boost::urls::ignore_case); pv.end() == pv_it)
        {
            LogDebug(Channel::Web, "URL was malformed and was missing expected query parameters");
        }
        else
        {
            return (*pv_it).value;
        }

        return tl::unexpected(false);
    }

}
// namespace AqualinkAutomate::HTTP
