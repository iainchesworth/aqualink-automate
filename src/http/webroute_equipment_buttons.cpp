#include <format>

#include "http/webroute_equipment_buttons.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Buttons::WebRoute_Equipment_Buttons(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>(http_server,
			{
				{ HTTP::Methods::GET, std::bind(&WebRoute_Equipment_Buttons::ButtonCollection_GetHandler, this, std::placeholders::_1, std::placeholders::_2) },
				{ HTTP::Methods::POST, std::bind(&WebRoute_Equipment_Buttons::ButtonCollection_PostHandler, this, std::placeholders::_1, std::placeholders::_2) }
			}
		),
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_BUTTON_ROUTE_URL>(http_server,
			{ 
				{ HTTP::Methods::GET, std::bind(&WebRoute_Equipment_Buttons::ButtonIndividual_GetHandler, this, std::placeholders::_1, std::placeholders::_2) },
				{ HTTP::Methods::POST, std::bind(&WebRoute_Equipment_Buttons::ButtonIndividual_PostHandler, this, std::placeholders::_1, std::placeholders::_2) }
			}
		),
		Interfaces::IShareableRoute(),
		m_Buttons
		{
			/*std::make_shared<TriggerableButton>(
				"poolHeat",
				std::bind(&WebRoute_Equipment_Buttons::Button_PoolHeatStatus, this),
				std::bind(&WebRoute_Equipment_Buttons::Button_PoolHeatTrigger, this, std::placeholders::_1) 
			),
			std::make_shared<TriggerableButton>(
				"spaHeat",
				std::bind(&WebRoute_Equipment_Buttons::Button_SpaHeatStatus, this),
				std::bind(&WebRoute_Equipment_Buttons::Button_SpaHeatTrigger, this, std::placeholders::_1)
			),
				std::make_shared<TriggerableButton>(
				"user",
				std::bind(&WebRoute_Equipment_Buttons::Button_UserStatus, this),
				std::bind(&WebRoute_Equipment_Buttons::Button_UserTrigger, this, std::placeholders::_1)
			),
			std::make_shared<TriggerableButton>(
				"clean",
				std::bind(&WebRoute_Equipment_Buttons::Button_CleanStatus, this),
				std::bind(&WebRoute_Equipment_Buttons::Button_CleanTrigger, this, std::placeholders::_1)
			)*/
		},
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Equipment_Buttons::ButtonCollection_GetHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		nlohmann::json all_buttons;

		for (auto& elem : m_Buttons())
		{
			all_buttons[elem->Id()] = elem->Status();
		}

		resp.set_status_and_content(
			cinatra::status_type::ok,
			all_buttons.dump(),
			cinatra::req_content_type::json,
			cinatra::content_encoding::none
		);
	}

	void WebRoute_Equipment_Buttons::ButtonCollection_PostHandler(HTTP::Request& req, HTTP::Response& resp)
	{
	}

	void WebRoute_Equipment_Buttons::ButtonIndividual_GetHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		const std::string button_id(req.get_query_value("button_id"));

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			Report_SystemIsInactive(resp);
		}
		else
		{
			auto it = std::find_if(m_Buttons().cbegin(), m_Buttons().cend(), [&button_id](auto& button) -> bool { return (button_id == button->Id()); });
			if (it == m_Buttons().cend())
			{
				Report_ButtonDoesntExist(resp, button_id);
			}
			else
			{
				resp.set_status_and_content(
					cinatra::status_type::ok,
					(*it)->Status(),
					cinatra::req_content_type::json,
					cinatra::content_encoding::none
				);
			}
		}
	}

	void WebRoute_Equipment_Buttons::ButtonIndividual_PostHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		const std::string button_id(req.get_query_value("button_id"));

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			Report_SystemIsInactive(resp);
		}
		else 
		{
			auto it = std::find_if(m_Buttons().cbegin(), m_Buttons().cend(), [&button_id](auto& button) -> bool { return (button_id == button->Id()); });
			if (it == m_Buttons().cend())
			{
				Report_ButtonDoesntExist(resp, button_id);
			}
			else
			{
				// Attempt to trigger the sequencing of the button's "command".
				(*it)->Trigger(nlohmann::json(req.body()));

				// Return the button's (new) state i.a.w. RESTful principles.
				resp.set_status_and_content(
					cinatra::status_type::ok,
					(*it)->Status(),
					cinatra::req_content_type::json,
					cinatra::content_encoding::none
				);
			}
		}
	}

	HTTP::TriggerableButtons& WebRoute_Equipment_Buttons::Buttons()
	{
		return m_Buttons;
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_PoolHeatStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_PoolHeatTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'poolHeat' action request received");
		return nlohmann::json();
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_SpaHeatStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_SpaHeatTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'spaHeat' action request received");
		return nlohmann::json();
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_UserStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_UserTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'user' action request received");
		return nlohmann::json();
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_CleanStatus()
	{
		return nlohmann::json();
	}

	nlohmann::json WebRoute_Equipment_Buttons::Button_CleanTrigger(const nlohmann::json payload)
	{
		LogDebug(Channel::Web, "'clean' action request received");
		return nlohmann::json();
	}

	void WebRoute_Equipment_Buttons::Report_ButtonDoesntExist(HTTP::Response& resp, const std::string& button_id)
	{
		LogInfo(Channel::Web, std::format("Received an invalid button id ('{}'); rejecting button request", button_id));

		resp.set_status_and_content(
			cinatra::status_type::not_found,
			std::format("'{}' is an invalid button id", button_id),
			cinatra::req_content_type::text,
			cinatra::content_encoding::none
		);
	}

	void WebRoute_Equipment_Buttons::Report_ButtonIsInactive(HTTP::Response& resp, const std::string& button_id)
	{
	}

	void WebRoute_Equipment_Buttons::Report_SystemIsInactive(HTTP::Response& resp)
	{
		LogInfo(Channel::Web, "Aqualink Automate has not yet initialised (PoolConfiguration == Unknown); rejecting button action request");

		resp.set_status_and_content(
			cinatra::status_type::service_unavailable,
			"Service is not initialised; cannot action button",
			cinatra::req_content_type::text,
			cinatra::content_encoding::none
		);

		resp.add_header("Content-Type", "text/plain");
		resp.add_header("Retry-After", "30");
	}

}
// namespace AqualinkAutomate::HTTP
