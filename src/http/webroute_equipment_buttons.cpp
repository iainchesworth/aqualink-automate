#include <algorithm>
#include <format>
#include <ranges>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>

#include "http/webroute_equipment_buttons.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Buttons::WebRoute_Equipment_Buttons(HTTP::Server& http_server, Kernel::HubLocator& hub_locator) :
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
		Interfaces::IShareableRoute()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
	}

	void WebRoute_Equipment_Buttons::ButtonCollection_GetHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		nlohmann::json buttons, all_buttons;

		const auto all_devices = m_DataHub->Devices.FindByTrait(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{});
		std::for_each(all_devices.begin(), all_devices.end(), [&buttons](const auto& device)
			{
				nlohmann::json button;

				button["id"] = boost::uuids::to_string(device->Id());
				
				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}))
				{ 
					button["label"] = *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}]);
				}
				
				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::StatusTrait{}))
				{
					button["status"] = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device);
				}

				buttons.push_back(button);
			}
		);

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

		try
		{
			if (Kernel::PoolConfigurations::Unknown == m_DataHub->PoolConfiguration)
			{
				Report_SystemIsInactive(resp);
			}
			else if (const auto device{ m_DataHub->Devices.FindById(boost::uuids::string_generator()(button_id)) }; nullptr == device)
			{
				// Invalid device pointer...return a bad status.
				Report_ButtonDoesntExist(resp, button_id);
			}
			else
			{
				nlohmann::json button;

				button["id"] = button_id;

				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::LabelTrait{}))
				{
					button["label"] = *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::LabelTrait{}]);
				}

				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::StatusTrait{}))
				{
					button["status"] = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device);
				}

				resp.set_status_and_content
				(
					cinatra::status_type::ok,
					button.dump(),
					cinatra::req_content_type::json,
					cinatra::content_encoding::none
				);
			}
		}
		catch (const std::runtime_error& exRE)
		{
			// Raised if the UUID is malformed and cannot be parsed.
			Report_ButtonDoesntExist(resp, button_id);
		}
	}

	void WebRoute_Equipment_Buttons::ButtonIndividual_PostHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		const auto button_id_sv{ req.get_matches()[1] };
		const std::string button_id{button_id_sv.str()};

		if (Kernel::PoolConfigurations::Unknown == m_DataHub->PoolConfiguration)
		{
			Report_SystemIsInactive(resp);
		}
		else 
		{
			try
			{
				if (button_id.empty())
				{
					Report_ButtonDoesntExist(resp, "");
				}
				else if (const auto button_device{ m_DataHub->Devices.FindById(boost::uuids::string_generator()(button_id)) }; nullptr == button_device)
				{
					Report_ButtonDoesntExist(resp, button_id);
				}
				else
				{
					// Trigger the device...then get its updated status and report that back.

					{
						///FIXME -> Trigger the device...
						///button_device -> ...
					}

					ButtonIndividual_GetHandler(req, resp);
				}
			}
			catch (const std::runtime_error& exRE)
			{
				// The Boost UUID generator failed as the string was invalid / incorrectly formatted.
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
