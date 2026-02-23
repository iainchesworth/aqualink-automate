#pragma once

#include <functional>
#include <optional>

#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/cobalt/detached.hpp>

#include "http/server/server_types.h"

namespace AqualinkAutomate::HTTP
{
	boost::cobalt::detached DetectSession(TcpStream stream, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context, boost::asio::cancellation_slot cancellation_slot);

}
// namespace AqualinkAutomate::HTTP
