#include "http/server/response_404.h"

namespace AqualinkAutomate::HTTP
{

    HTTP::Message Response_404(HTTP::Request const& req)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
        res.set(boost::beast::http::field::server, "1.2.3.4");
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(req.target()) + "' was not found.";
        res.prepare_payload();

        return res;
    }

} 
// namespace AqualinkAutomate::HTTP