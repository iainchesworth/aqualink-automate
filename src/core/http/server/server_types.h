#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

namespace AqualinkAutomate::HTTP
{

    using Message = boost::beast::http::message_generator;
    using Request = boost::beast::http::request<boost::beast::http::string_body>;
    using Response = boost::beast::http::response<boost::beast::http::string_body>;

    using Status = boost::beast::http::status;
    using Verbs = boost::beast::http::verb;

    using TcpAcceptor = boost::asio::ip::tcp::acceptor;
    using TcpSocket = boost::asio::ip::tcp::socket;

    using TcpStream = boost::beast::tcp_stream;
    using SslStream = boost::beast::ssl_stream<TcpStream>;

    using WsStream = boost::beast::websocket::stream<TcpStream>;
    using WsSslStream = boost::beast::websocket::stream<SslStream>;

}
// namespace AqualinkAutomate::HTTP
