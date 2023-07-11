#pragma once

#include <functional>
#include <string_view>
#include <vector>

#include <mstch/mstch.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_types.h"
#include "interfaces/ishareableroute.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTBUTTONS_ROUTE_URL[] = "/api/equipment/buttons";
	inline constexpr char EQUIPMENTBUTTONS_BUTTON_ROUTE_URL[] = "/api/equipment/buttons/{:button_id}";

	class WebRoute_Equipment_Buttons : public Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>, public Interfaces::IWebRoute<EQUIPMENTBUTTONS_BUTTON_ROUTE_URL>, public Interfaces::IShareableRoute
	{
	public:
		WebRoute_Equipment_Buttons(HTTP::Server& http_server, const Kernel::DataHub& data_hub);

	public:
		void ButtonCollection_GetHandler(HTTP::Request& req, HTTP::Response& resp);
		void ButtonCollection_PostHandler(HTTP::Request& req, HTTP::Response& resp);

	public:
		void ButtonIndividual_GetHandler(HTTP::Request& req, HTTP::Response& resp);
		void ButtonIndividual_PostHandler(HTTP::Request& req, HTTP::Response& resp);

	private:
		void Report_ButtonDoesntExist(HTTP::Response& resp, const std::string& button_id);
		void Report_ButtonIsInactive(HTTP::Response& resp, const std::string& button_id);
		void Report_SystemIsInactive(HTTP::Response& resp);

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
