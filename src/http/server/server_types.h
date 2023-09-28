#pragma once

#include <functional>
#include <string_view>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>

namespace AqualinkAutomate::HTTP
{

    using Message = boost::beast::http::message_generator;
    using Request = boost::beast::http::request<boost::beast::http::string_body>;
    using Response = boost::beast::http::response<boost::beast::http::string_body>;

    using Status = boost::beast::http::status;
    using Verbs = boost::beast::http::verb;

} 
// namespace AqualinkAutomate::HTTP
