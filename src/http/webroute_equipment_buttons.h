#pragma once

#include <functional>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTBUTTONS_ROUTE_URL[] = "/api/equipment/buttons";
	constexpr const char EQUIPMENTBUTTONS_BUTTON_ROUTE_URL[] = "/api/equipment/buttons/{:button_id}";

	class WebRoute_Equipment_Buttons : public Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>, public Interfaces::IWebRoute<EQUIPMENTBUTTONS_BUTTON_ROUTE_URL>
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
		struct TriggerableButton
		{
			const std::string device_id;
			std::function<nlohmann::json()> device_status;
			std::function<nlohmann::json(const nlohmann::json payload)> device_trigger;
		};

	private:
		std::vector<TriggerableButton> m_Buttons;

	private:
		nlohmann::json Button_PoolHeatStatus();
		nlohmann::json Button_PoolHeatTrigger(const nlohmann::json payload);
		nlohmann::json Button_SpaHeatStatus();
		nlohmann::json Button_SpaHeatTrigger(const nlohmann::json payload);
		nlohmann::json Button_UserStatus();
		nlohmann::json Button_UserTrigger(const nlohmann::json payload);
		nlohmann::json Button_CleanStatus();
		nlohmann::json Button_CleanTrigger(const nlohmann::json payload);

	private:
		void Report_ButtonDoesntExist(HTTP::Response& resp, const std::string& button_id);
		void Report_ButtonIsInactive(HTTP::Response& resp, const std::string& button_id);
		void Report_SystemIsInactive(HTTP::Response& resp);

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
