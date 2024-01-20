#include <format>

#include <magic_enum.hpp>

#include "formatters/beast_stringview_formatter.h"
#include "http/server/server_fields.h"
#include "http/server/responses/response_400.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP::Responses
{

    HTTP::Response Response_400(HTTP::Request const& req)
    {
        LogTrace(Channel::Web, std::format("Responding 400 BAD REQUEST to HTTP {} request for {}", magic_enum::enum_name(req.method()), req.target()));

        boost::beast::http::response<boost::beast::http::string_body> res
        { 
            boost::beast::http::status::bad_request, 
            req.version() 
        };

        res.set(boost::beast::http::field::server, ServerFields::Server());
        res.set(boost::beast::http::field::content_type, ContentTypes::TEXT_HTML);
        res.keep_alive(req.keep_alive());
        res.body() = "Bad request: '" + std::string(req.target()) + "'.";
        res.prepare_payload();

        return res;
    }

}
// namespace AqualinkAutomate::HTTP::Responses
