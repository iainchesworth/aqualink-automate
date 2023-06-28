#include <format>

#include "http/webroute_jandyequipment_buttons.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment_Buttons::WebRoute_JandyEquipment_Buttons(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>(app,
			{
				{ crow::HTTPMethod::Get, std::bind(&WebRoute_JandyEquipment_Buttons::ButtonCollection_GetHandler, this, std::placeholders::_1, std::placeholders::_2) },
				{ crow::HTTPMethod::Post, std::bind(&WebRoute_JandyEquipment_Buttons::ButtonCollection_PostHandler, this, std::placeholders::_1, std::placeholders::_2) }
			}
		),
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_BUTTON_ROUTE_URL, Buttons::ButtonRouteHandler>(app,
			{ 
				{ crow::HTTPMethod::Get, std::bind(&WebRoute_JandyEquipment_Buttons::ButtonIndividual_GetHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) },
				{ crow::HTTPMethod::Post, std::bind(&WebRoute_JandyEquipment_Buttons::ButtonIndividual_PostHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) },
			}
		),
		m_Buttons 
		{
			TriggerableButton{
				"poolHeat",
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_PoolHeatStatus, this),
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_PoolHeatTrigger, this, std::placeholders::_1) 
			},
			TriggerableButton{
				"spaHeat",
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_SpaHeatStatus, this),
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_SpaHeatTrigger, this, std::placeholders::_1)
			},
			TriggerableButton{
				"user",
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_UserStatus, this),
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_UserTrigger, this, std::placeholders::_1)
			},
			TriggerableButton{
				"clean",
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_CleanStatus, this),
				std::bind(&WebRoute_JandyEquipment_Buttons::Button_CleanTrigger, this, std::placeholders::_1)
			}
		},
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebRoute_JandyEquipment_Buttons::ButtonCollection_GetHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		nlohmann::json all_buttons;

		for (auto& elem : m_Buttons)
		{
			all_buttons[elem.device_id] = elem.device_status();
		}
		
		resp.set_header("Content-Type", "application/json");		
		resp.body = all_buttons.dump();
		resp.end();
	}

	void WebRoute_JandyEquipment_Buttons::ButtonCollection_PostHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
	}

	void WebRoute_JandyEquipment_Buttons::ButtonIndividual_GetHandler(const HTTP::Request& req, HTTP::Response& resp, const std::string& button_id)
	{
		if (Config::PoolConfigurations::Unknown == m_JandyEquipment.Config().PoolConfiguration)
		{
			Report_SystemIsInactive(resp);
		}
		else
		{
			auto it = std::find_if(m_Buttons.begin(), m_Buttons.end(), [&button_id](auto& button) -> bool { return (button_id == button.device_id); });
			if (it == m_Buttons.end())
			{
				Report_ButtonDoesntExist(resp, button_id);
			}
			else
			{
				resp.set_header("Content-Type", "application/json");
				resp.code = 200;
				resp.body = it->device_status().dump();
			}
		}

		resp.end();
	}

	void WebRoute_JandyEquipment_Buttons::ButtonIndividual_PostHandler(const HTTP::Request& req, HTTP::Response& resp, const std::string& button_id)
	{
		if (Config::PoolConfigurations::Unknown == m_JandyEquipment.Config().PoolConfiguration)
		{
			Report_SystemIsInactive(resp);
		}
		else 
		{
			auto it = std::find_if(m_Buttons.begin(), m_Buttons.end(), [&button_id](auto& button) -> bool { return (button_id == button.device_id); });
			if (it == m_Buttons.end())
			{
				Report_ButtonDoesntExist(resp, button_id);
			}
			else
			{
				// Attempt to trigger the sequencing of the button's "command".
				it->device_trigger(nlohmann::json(req.body));

				// Return the button's (new) state i.a.w. RESTful principles.
				resp.set_header("Content-Type", "application/json");
				resp.code = 200;
				resp.body = it->device_status().dump();
			}
		}

		resp.end();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_PoolHeatStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_PoolHeatTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'poolHeat' action request received");
		return nlohmann::json();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_SpaHeatStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_SpaHeatTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'spaHeat' action request received");
		return nlohmann::json();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_UserStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_UserTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'user' action request received");
		return nlohmann::json();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_CleanStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_JandyEquipment_Buttons::Button_CleanTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'clean' action request received");
		return nlohmann::json();
	}

	void WebRoute_JandyEquipment_Buttons::Report_ButtonDoesntExist(HTTP::Response& resp, const std::string& button_id)
	{
		LogInfo(Channel::Web, std::format("Received an invalid button id ('{}'); rejecting button request", button_id));

		resp.set_header("Content-Type", "text/plain");
		resp.code = 404;
		resp.body = std::format("'{}' is an invalid button id", button_id);
	}

	void WebRoute_JandyEquipment_Buttons::Report_ButtonIsInactive(HTTP::Response& resp, const std::string& button_id)
	{
	}

	void WebRoute_JandyEquipment_Buttons::Report_SystemIsInactive(HTTP::Response& resp)
	{
		LogInfo(Channel::Web, "Aqualink Automate has not yet initialised (PoolConfiguration == Unknown); rejecting button action request");

		resp.set_header("Content-Type", "text/plain");
		resp.set_header("Retry-After", "30");
		resp.code = 503;
		resp.body = "Service is not initialised; cannot action button";
	}

}
// namespace AqualinkAutomate::HTTP
