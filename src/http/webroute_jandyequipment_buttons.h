#pragma once

#include <functional>
#include <string_view>
#include <vector>

#include <crow/app.h>
#include <nlohmann/json.hpp>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTBUTTONS_ROUTE_URL[] = "/api/equipment/buttons";
	constexpr const char EQUIPMENTBUTTONS_BUTTON_ROUTE_URL[] = "/api/equipment/buttons/<string>";

	namespace Buttons
	{
		using ButtonRouteHandler = std::function<void(const HTTP::Request&, HTTP::Response&, const std::string&)>;
	}
	// namespace Buttons

	class WebRoute_JandyEquipment_Buttons : public Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>, public Interfaces::IWebRoute<EQUIPMENTBUTTONS_BUTTON_ROUTE_URL, Buttons::ButtonRouteHandler>
	{
	public:
		WebRoute_JandyEquipment_Buttons(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void ButtonCollection_GetHandler(const HTTP::Request& req, HTTP::Response& resp);
		void ButtonCollection_PostHandler(const HTTP::Request& req, HTTP::Response& resp);

	public:
		void ButtonIndividual_GetHandler(const HTTP::Request& req, HTTP::Response& resp, const std::string& button_id);
		void ButtonIndividual_PostHandler(const HTTP::Request& req, HTTP::Response& resp, const std::string& button_id);

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
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
