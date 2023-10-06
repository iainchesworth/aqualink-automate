#include <format>

#include <boost/beast/http/file_body.hpp>
#include <magic_enum.hpp>

#include "formatters/beast_stringview_formatter.h"
#include "http/server/responses/response_staticfile.h"
#include "http/server/responses/response_400.h"
#include "http/server/responses/response_404.h"
#include "http/server/responses/response_500.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP::Responses
{
    inline constexpr auto hash_djb2a(const std::string_view sv) 
    {
        unsigned long hash{ 5381 };

        for (unsigned char c : sv) 
        {
            hash = ((hash << 5) + hash) ^ c;
        }

        return hash;
    }

    inline constexpr auto operator"" _sh(const char* str, std::size_t len) 
    {
        return hash_djb2a(std::string_view{ str, len });
    }

    HTTP::Message Response_StaticFile(HTTP::Request const& req, const std::filesystem::path& result)
    {
        auto mime_type = [](std::string_view path) -> std::string_view
            {
                auto const ext = [&path]() -> std::string_view
                    {
                        auto const pos = path.rfind(".");
                        if (pos == std::string_view::npos)
                        {
                            return std::string_view{};
                        }

                        return path.substr(pos);
                    }
                ();

                switch (hash_djb2a(ext))
                {
                case ".htm"_sh:
                    [[fallthrough]];
                case ".html"_sh:
                    return "text/html";

                case ".css"_sh:
                    return "text/css";

                case ".js"_sh:
                    return "application/javascript";

                default:
                    return "application/text";
                }
            };

        LogTrace(Channel::Web, std::format("Responding to HTTP {} request for {} (file: {})", magic_enum::enum_name(req.method()), req.target(), result.string()));

        boost::beast::error_code ec;
        boost::beast::http::file_body::value_type body;

        if (req.target().empty() || req.target()[0] != '/' || std::string_view::npos != req.target().find(".."))
        {
            LogDebug(Channel::Web, std::format("Requested target ({}) is not permitted", req.target()));
            return HTTP::Responses::Response_400(req);
        }
        else if (body.open(result.string().c_str(), boost::beast::file_mode::scan, ec); boost::beast::errc::no_such_file_or_directory == ec)
        {
            LogDebug(Channel::Web, std::format("Requested target ({}) file ({}) does not exist", req.target(), result.string()));
            return HTTP::Responses::Response_404(req);
        }
        else if (ec)
        {
            LogDebug(Channel::Web, std::format("Error occured while handling static file ({}) request; error was -> {}", result.string(), ec.message()));
            return HTTP::Responses::Response_500(req);
        }
        else
        {
            // Cache the size since we need it after the move
            auto const size = body.size();

            boost::beast::http::response<boost::beast::http::file_body> res
            {
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(boost::beast::http::status::ok, req.version())
            };

            res.set(boost::beast::http::field::server, "1.2.3.4");
            res.set(boost::beast::http::field::content_type, mime_type(result.string()));
            res.content_length(size);
            res.keep_alive(req.keep_alive());

            return res;           
        }
    }

}
// namespace AqualinkAutomate::HTTP::Responses
