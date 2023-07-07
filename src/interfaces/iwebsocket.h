#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "concepts/is_c_array.h"
#include "http/webroute_types.h"

namespace AqualinkAutomate::Interfaces
{

	template<const auto& ROUTE_URL>
	requires (Concepts::CArrayRef<decltype(ROUTE_URL)>)
	class IWebSocket
	{
	public:
		explicit IWebSocket(HTTP::Server& http_server) :
			m_OnActionMutex(),
			m_Connections()
		{
			http_server.set_http_handler<HTTP::Methods::GET, HTTP::Methods::POST>(
				ROUTE_URL,
				[this](HTTP::Request& req, HTTP::Response& res)
				{
					HandleWebSocketConnection(req, res);
				}
			);
		}

	private:
		void HandleWebSocketConnection(HTTP::Request& req, HTTP::Response& resp)
		{
			if (req.get_content_type() != cinatra::content_type::websocket)
			{
				///FIXME
			}
			else
			{
				req.on(cinatra::ws_open,	std::bind(&IWebSocket::HandleWebSocket_OnOpen, this, std::placeholders::_1));
				req.on(cinatra::ws_message, std::bind(&IWebSocket::HandleWebSocket_OnMessage, this, std::placeholders::_1));
				req.on(cinatra::ws_close,	std::bind(&IWebSocket::HandleWebSocket_OnClose, this, std::placeholders::_1));
				req.on(cinatra::ws_error,	std::bind(&IWebSocket::HandleWebSocket_OnError, this, std::placeholders::_1));
			}
		}

	private:
		void HandleWebSocket_OnOpen(HTTP::Request& req)
		{
			const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);

			auto connection = req.get_conn<cinatra::NonSSL>();
			m_Connections.emplace(0, connection);

			OnOpen(req);
		}

		void HandleWebSocket_OnMessage(HTTP::Request& req)
		{
			const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);
			OnMessage(req);
		}

		void HandleWebSocket_OnClose(HTTP::Request& req)
		{
			const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);
			OnClose(req);
		}

		void HandleWebSocket_OnError(HTTP::Request& req)
		{
			const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);
			OnError(req);
		}

	protected:
		virtual void OnOpen(HTTP::Request& req) = 0;
		virtual void OnMessage(HTTP::Request& req) = 0;
		virtual void OnClose(HTTP::Request& req) = 0;
		virtual void OnError(HTTP::Request& req) = 0;

	protected:
		void PublishMessage_AsBinary(const std::string& message)
		{
			for (auto& [id, conn] : m_Connections)
			{
				if (nullptr != conn)
				{
					conn->send_ws_binary(message);
				}
			}
		}

		void PublishMessage_AsText(const std::string& message)
		{
			for (auto& [id, conn] : m_Connections)
			{
				if (nullptr != conn)
				{
					conn->send_ws_string(message);
				}
			}
		}

	private:
		std::unordered_map<uint32_t, HTTP::Connection> m_Connections;

	private:
		std::mutex m_OnActionMutex;
	};
}
// namespace AqualinkAutomate::Interfaces
