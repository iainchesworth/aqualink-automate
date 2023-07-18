#include <format>

#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>

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
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Equipment_Buttons::ButtonCollection_GetHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		auto generate_button_json = [](auto button_ptr) -> nlohmann::json
		{
			nlohmann::json button;
			button["id"] = boost::uuids::to_string(button_ptr->Id());
			button["label"] = button_ptr->Label();
			button["status"] = magic_enum::enum_name(button_ptr->Status());
			return button;
		};

		nlohmann::json buttons;

		for (auto& elem : m_DataHub.Auxillaries())
		{
			buttons.push_back(generate_button_json(elem));
		}

		nlohmann::json all_buttons;
		all_buttons["buttons"] = buttons;

		resp.set_status_and_content(
			cinatra::status_type::ok,
			all_buttons.dump(),
			cinatra::req_content_type::json,
			cinatra::content_encoding::none
		);
	}

	void WebRoute_Equipment_Buttons::ButtonCollection_PostHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		resp.set_status_and_content(
			cinatra::status_type::not_implemented,
			std::string{},
			cinatra::req_content_type::text,
			cinatra::content_encoding::none
		);
	}

	void WebRoute_Equipment_Buttons::ButtonIndividual_GetHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		const auto button_id_sv{req.get_matches()[1]};
		const std::string button_id{button_id_sv.str()};

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			Report_SystemIsInactive(resp);
		}
		else
		{
			auto check_for_device_and_return_status = [&resp](auto collection, auto& button_id) -> bool
			{
				auto match_via_shared_ptr = [&button_id](const auto& button_ptr) -> bool
				{
					if (nullptr == button_ptr)
					{
						return false;
					}
					else if (auto ptr = std::dynamic_pointer_cast<Kernel::AuxillaryBase>(button_ptr); nullptr == ptr)
					{
						return false;
					}
					else
					{
						return ((*ptr) == button_id);
					}
				};

				if (auto it = std::find_if(collection.cbegin(), collection.cend(), match_via_shared_ptr); collection.cend() == it)
				{
					return false;
				}
				else
				{
					nlohmann::json button;
					button["label"] = (*it)->Label();
					button["status"] = magic_enum::enum_name((*it)->Status());

					resp.set_status_and_content
					(
						cinatra::status_type::ok, 
						button.dump(),
						cinatra::req_content_type::json, 
						cinatra::content_encoding::none
					);

					return true;
				}
			};

			if (button_id.empty())
			{
				Report_ButtonDoesntExist(resp, "");
			}
			else if (check_for_device_and_return_status(m_DataHub.Auxillaries(), button_id))
			{
				// Found the device --> it's an auxillary
			}
			else if (check_for_device_and_return_status(m_DataHub.Heaters(), button_id))
			{
				// Found the device --> it's an heater
			}
			else if (check_for_device_and_return_status(m_DataHub.Pumps(), button_id))
			{
				// Found the device --> it's an pump
			}
			else
			{
				Report_ButtonDoesntExist(resp, button_id);
			}
		}
	}

	void WebRoute_Equipment_Buttons::ButtonIndividual_PostHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		const auto button_id_sv{ req.get_matches()[1] };
		const std::string button_id{button_id_sv.str()};

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			Report_SystemIsInactive(resp);
		}
		else 
		{
			auto check_for_device_then_trigger_it_and_return_status = [this, &req, &resp](auto collection, auto& button_id) -> bool
			{
				{
					///FIXME -> Trigger the device...
				}

				ButtonIndividual_GetHandler(req, resp);

				return true;
			};

			if (button_id.empty())
			{
				Report_ButtonDoesntExist(resp, "");
			}
			else if (check_for_device_then_trigger_it_and_return_status(m_DataHub.Auxillaries(), button_id))
			{
				// Found the device --> it's an auxillary
			}
			else if (check_for_device_then_trigger_it_and_return_status(m_DataHub.Heaters(), button_id))
			{
				// Found the device --> it's an heater
			}
			else if (check_for_device_then_trigger_it_and_return_status(m_DataHub.Pumps(), button_id))
			{
				// Found the device --> it's an pump
			}
			else
			{
				Report_ButtonDoesntExist(resp, button_id);
			}
		}
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
