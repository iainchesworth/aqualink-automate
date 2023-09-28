#include <algorithm>
#include <format>
#include <ranges>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>

#include "http/webroute_equipment_buttons.h"
#include "http/server/parse_query_string.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Buttons::WebRoute_Equipment_Buttons(Kernel::HubLocator& hub_locator) :
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>() // ,
		// Interfaces::IWebRoute<EQUIPMENTBUTTONS_BUTTON_ROUTE_URL>()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
    }

    HTTP::Message WebRoute_Equipment_Buttons::OnRequest(HTTP::Request req)
    {
		///FIXME
        return ButtonCollection_GetHandler(req);
    }

	HTTP::Message WebRoute_Equipment_Buttons::ButtonCollection_GetHandler(HTTP::Request req)
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

        HTTP::Response resp{HTTP::Status::ok, req.version()};

        resp.set(boost::beast::http::field::server, "1.2.3.4");
        resp.set(boost::beast::http::field::content_type, "application/json");
        resp.keep_alive(req.keep_alive());
        resp.body() = all_buttons.dump();
        resp.prepare_payload();

        return resp;
	}

	HTTP::Message WebRoute_Equipment_Buttons::ButtonCollection_PostHandler(HTTP::Request req)
    {
        HTTP::Response resp{HTTP::Status::ok, req.version()};

        resp.set(boost::beast::http::field::server, "1.2.3.4");
        resp.set(boost::beast::http::field::content_type, "text/html");
        resp.keep_alive(req.keep_alive());
        resp.body() = std::string("");
        resp.prepare_payload();

        return resp;
	}

	HTTP::Message WebRoute_Equipment_Buttons::ButtonIndividual_GetHandler(HTTP::Request req)
	{
        const std::string UNKNOWN_BUTTON_ID{"Unknown Or Missing Button Id"};

		try
		{
			if (Kernel::PoolConfigurations::Unknown == m_DataHub->PoolConfiguration)
			{
				return Report_SystemIsInactive(req);
			}
            else if (auto button_id = HTTP::ParseQueryString(req, "button_id"); !button_id.has_value())
			{
                return Report_ButtonDoesntExist(req, UNKNOWN_BUTTON_ID);
			}
			else if (const auto device{ m_DataHub->Devices.FindById(boost::uuids::string_generator()(button_id.value())) }; nullptr == device)
			{
				// Invalid device pointer...return a bad status.
                return Report_ButtonDoesntExist(req, button_id.value());
			}
			else
			{
				nlohmann::json button;

				button["id"] = button_id.value();

				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::LabelTrait{}))
				{
					button["label"] = *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::LabelTrait{}]);
				}

				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::StatusTrait{}))
				{
					button["status"] = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device);
                }

                HTTP::Response resp{HTTP::Status::ok, req.version()};

                resp.set(boost::beast::http::field::server, "1.2.3.4");
                resp.set(boost::beast::http::field::content_type, "application/json");
                resp.keep_alive(req.keep_alive());
                resp.body() = button.dump();
                resp.prepare_payload();

                return resp;
			}
		}
		catch (const std::runtime_error& ex_re)
		{
			// Raised if the UUID is malformed and cannot be parsed.
            return Report_ButtonDoesntExist(req, UNKNOWN_BUTTON_ID);
		}
	}

	HTTP::Message WebRoute_Equipment_Buttons::ButtonIndividual_PostHandler(HTTP::Request req)
    {
        const std::string UNKNOWN_BUTTON_ID{"Unknown Or Missing Button Id"};

		if (Kernel::PoolConfigurations::Unknown == m_DataHub->PoolConfiguration)
		{
            return Report_SystemIsInactive(req);
		}
		else 
		{
			try
            {
                if (auto button_id = HTTP::ParseQueryString(req, "button_id"); !button_id.has_value())
                {
                    return Report_ButtonDoesntExist(req, UNKNOWN_BUTTON_ID);
                }
				else if (const auto button_device{ m_DataHub->Devices.FindById(boost::uuids::string_generator()(button_id.value())) }; nullptr == button_device)
				{
                    return Report_ButtonDoesntExist(req, button_id.value());
				}
				else
				{
					// Trigger the device...then get its updated status and report that back.

					{
						///FIXME -> Trigger the device...
						///button_device -> ...
					}

					return ButtonIndividual_GetHandler(req);
				}
			}
			catch (const std::runtime_error& ex_re)
			{
				// The Boost UUID generator failed as the string was invalid / incorrectly formatted.
                return Report_ButtonDoesntExist(req, UNKNOWN_BUTTON_ID);
			}			
		}
	}

	HTTP::Message WebRoute_Equipment_Buttons::Report_ButtonDoesntExist(HTTP::Request req, const std::string& button_id)
	{
		LogInfo(Channel::Web, std::format("Received an invalid button id ('{}'); rejecting button request", button_id));

		HTTP::Response resp{HTTP::Status::not_found, req.version()};

        resp.set(boost::beast::http::field::server, "1.2.3.4");
        resp.set(boost::beast::http::field::content_type, "text/plain");
        resp.set(boost::beast::http::field::content_encoding, "none");
        resp.keep_alive(req.keep_alive());
        resp.body() = std::format("'{}' is an invalid button id", button_id);
        resp.prepare_payload();

        return resp;
	}

	HTTP::Message WebRoute_Equipment_Buttons::Report_ButtonIsInactive(HTTP::Request req, const std::string& button_id)
	{
		///FIXME
        throw;
	}

	HTTP::Message WebRoute_Equipment_Buttons::Report_SystemIsInactive(HTTP::Request req)
	{
        LogInfo(Channel::Web, "Aqualink Automate has not yet initialised (PoolConfiguration == Unknown); rejecting button action request");

        HTTP::Response resp{HTTP::Status::service_unavailable, req.version()};

        resp.set(boost::beast::http::field::server, "1.2.3.4");
        resp.set(boost::beast::http::field::content_type, "text/plain");
        resp.set(boost::beast::http::field::retry_after, "30");
		resp.keep_alive(req.keep_alive());
        resp.body() = "Service is not initialised; cannot action button";
        resp.prepare_payload();

        return resp;
	}

}
// namespace AqualinkAutomate::HTTP
