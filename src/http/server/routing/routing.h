#pragma once

#include <memory>
#include <string_view>

#include "http/server/server_types.h"
#include "http/server/static_file_handler.h"
#include "interfaces/isession.h"
#include "interfaces/iwebroute.h"
#include "interfaces/iwebsocket.h"

namespace AqualinkAutomate::HTTP::Routing
{

	void Add(std::unique_ptr<Interfaces::IWebRouteBase>&& handler);
	void Add(std::unique_ptr<Interfaces::IWebSocketBase>&& handler);

	void StaticHandler(StaticFileHandler&& sf);

	HTTP::Message HTTP_OnRequest(const HTTP::Request& req);
	Interfaces::IWebSocketBase* WS_OnAccept(const std::string_view target);

}
// namespace AqualinkAutomate::HTTP::Routing
