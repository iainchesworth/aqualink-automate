#pragma once

#include <functional>
#include <mutex>
#include <string>

#include <crow/app.h>
#include <crow/routing.h>

#include "concepts/is_c_array.h"

namespace AqualinkAutomate::Interfaces
{	

	template<const auto& ROUTE_URL>
	requires (Concepts::CArrayRef<decltype(ROUTE_URL)>)
	class IWebSocket
	{
	public:
		using Connection = crow::websocket::connection;

	public:
		explicit IWebSocket(crow::SimpleApp& app) :
			m_OnActionMutex()
		{
			//CROW_WEBSOCKET_ROUTE(app, ROUTE_URL)
			app.route<crow::black_magic::get_parameter_tag(ROUTE_URL)>(ROUTE_URL).template websocket<std::remove_reference<decltype(app)>::type>(&app)
				.onopen([&](crow::websocket::connection& conn) 
					{ 
						const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);
						OnOpen(conn); 
					})
				.onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary)
					{
						const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);
						OnMessage(conn, data, is_binary);
					})
				.onclose([&](crow::websocket::connection& conn, const std::string& reason) 
					{
						const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);
						OnClose(conn, reason); 
					})
				.onerror([&](crow::websocket::connection& conn, const std::string& reason)
					{
						const std::lock_guard<std::mutex> action_lock(m_OnActionMutex);
						OnError(conn);
					});
		}

	protected:
		virtual void OnOpen(Connection& conn) = 0;
		virtual void OnMessage(Connection& conn, const std::string& data, bool is_binary) = 0;
		virtual void OnClose(Connection& conn, const std::string& reason) = 0;
		virtual void OnError(Connection& conn) = 0;

	private:
		std::mutex m_OnActionMutex;
	};
}
// namespace AqualinkAutomate::Interfaces
