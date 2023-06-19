#pragma once

#include <functional>
#include <string>

#include <crow/app.h>
#include <crow/routing.h>

namespace AqualinkAutomate::Interfaces
{

	template<const char* ROUTE_URL>
	class IWebSocket
	{
	public:
		using Connection = crow::websocket::connection;

	public:
		explicit IWebSocket(crow::SimpleApp& app)
		{
			app.route_dynamic(ROUTE_URL)
				.websocket()
				.onopen([&](crow::websocket::connection& conn) 
					{ 
						OnOpen(conn); 
					})
				.onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary)
					{
						OnMessage(conn, data, is_binary);
					})
				.onclose([&](crow::websocket::connection& conn, const std::string& reason) 
					{ 
						OnClose(conn, reason); 
					})
				.onerror([&](crow::websocket::connection& conn)
					{
						OnError(conn);
					});
		}

	private:
		virtual void OnOpen(Connection& conn) = 0;
		virtual void OnMessage(Connection& conn, const std::string& data, bool is_binary) = 0;
		virtual void OnClose(Connection& conn, const std::string& reason) = 0;
		virtual void OnError(Connection& conn) = 0;
	};
}
// namespace AqualinkAutomate::Interfaces
