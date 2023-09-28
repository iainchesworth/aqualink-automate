#include <boost/url.hpp>

#include "http/server/parse_query_string.h"

namespace AqualinkAutomate::HTTP
{

    tl::expected<std::string, bool> ParseQueryString(const Request& req, const std::string& query_parameter)
    {
        if (auto url = boost::urls::parse_origin_form(req.target()); url.has_error())
        {
            // MALFORMED URL
        }
        else if (auto pv = url->params(); !pv.contains(query_parameter, boost::urls::ignore_case))
        {
            // MISSING QUERY PARAM
        }
        else if (auto pv_it = pv.find(query_parameter, boost::urls::ignore_case); pv.end() == pv_it)
        {
            // MISSING QUERY PARAM
        }
        else
        {
            return (*pv_it).value;
        }

        return tl::unexpected(false);
    }

}
// namespace AqualinkAutomate::HTTP
