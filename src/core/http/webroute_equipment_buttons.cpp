#include <algorithm>
#include <format>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <magic_enum/magic_enum.hpp>

#include "http/webroute_equipment_buttons.h"
#include "http/server/parse_query_string.h"
#include "http/server/server_fields.h"
#include "http/server/responses/response_405.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Buttons::WebRoute_Equipment_Buttons(Kernel::HubLocator& hub_locator) :
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_PreferencesHub = hub_locator.Find<Kernel::PreferencesHub>();
    }

    HTTP::Message WebRoute_Equipment_Buttons::OnRequest(const HTTP::Request& req)
    {
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_EquipmentButtons::OnRequest", std::source_location::current());

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

		// User-friendly display names keyed by canonical label (empty if none).
		const nlohmann::json label_overrides = m_PreferencesHub ? m_PreferencesHub->LabelOverrides : nlohmann::json::object();

		const auto all_devices = m_DataHub->Devices.FindByTrait(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{});
		std::for_each(all_devices.begin(), all_devices.end(), [&buttons, &label_overrides](const auto& device)
			{
				nlohmann::json button;

				button["id"] = boost::uuids::to_string(device->Id());

				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::LabelTrait{}))
				{
					const std::string label = *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::LabelTrait{}]);
					button["label"] = label;

					// display_label = override for this canonical label, else the label.
					const auto it = label_overrides.is_object() ? label_overrides.find(label) : label_overrides.end();
					button["display_label"] = (it != label_overrides.end() && it->is_string()) ? it->get<std::string>() : label;
				}
				
				if (Kernel::AuxillaryTraitsTypes::HasStatus(device))
				{
					button["status"] = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device);
				}

				if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}))
				{
					button["device_type"] = std::string(magic_enum::enum_name(
						*(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}])
					));
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
