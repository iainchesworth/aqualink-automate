#include <algorithm>
#include <format>
#include <ranges>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>

#include "http/webroute_equipment_buttons.h"
#include "http/server/parse_query_string.h"
#include "http/server/server_fields.h"
#include "http/server/responses/response_405.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Buttons::WebRoute_Equipment_Buttons(Kernel::HubLocator& hub_locator) :
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
    }

    HTTP::Message WebRoute_Equipment_Buttons::OnRequest(const HTTP::Request& req)
    {
		switch (req.method())
		{
		case HTTP::Verbs::get:
			return ButtonCollection_GetHandler(req);

		case HTTP::Verbs::post:
			return ButtonCollection_PostHandler(req);

		default:
			return HTTP::Responses::Response_405(req);
		}
    }

	HTTP::Message WebRoute_Equipment_Buttons::ButtonCollection_GetHandler(const HTTP::Request& req)
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

        resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
        resp.keep_alive(req.keep_alive());
        resp.body() = all_buttons.dump();
        resp.prepare_payload();

        return resp;
	}

	HTTP::Message WebRoute_Equipment_Buttons::ButtonCollection_PostHandler(const HTTP::Request& req)
    {
        HTTP::Response resp{HTTP::Status::ok, req.version()};

        resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::TEXT_HTML);
        resp.keep_alive(req.keep_alive());
        resp.body() = std::string("");
        resp.prepare_payload();

        return resp;
	}

}
// namespace AqualinkAutomate::HTTP
