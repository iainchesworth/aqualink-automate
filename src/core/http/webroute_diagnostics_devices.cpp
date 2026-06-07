#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_devices.h"
#include "http/server/server_fields.h"
#include "interfaces/idescribable.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_Diagnostics_Devices::WebRoute_Diagnostics_Devices(Kernel::HubLocator& hub_locator)
	{
		m_EquipmentHub = hub_locator.Find<Kernel::EquipmentHub>();
	}

	boost::beast::http::message_generator WebRoute_Diagnostics_Devices::OnRequest(const HTTP::Request& req)
	{
		if (req.method() != Verbs::get)
		{
			HTTP::Response resp{ HTTP::Status::method_not_allowed, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
			resp.keep_alive(req.keep_alive());
			resp.body() = R"({"error":"Method not allowed. Use GET."})";
			resp.prepare_payload();
			return resp;
		}

		nlohmann::json result = nlohmann::json::array();

		m_EquipmentHub->ForEachDevice([&result](const Interfaces::IDevice& device)
			{
				// Only emulated devices inherit IDescribable, so this cast
				// implicitly filters to emulated devices.
				auto describable = dynamic_cast<const Interfaces::IDescribable*>(&device);
				if (!describable)
				{
					return;
				}

				result.push_back(describable->DescribeDiagnostics());
			});

		HTTP::Response resp{ HTTP::Status::ok, req.version() };
		resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
		resp.keep_alive(req.keep_alive());
		resp.body() = result.dump();
		resp.prepare_payload();

		return resp;
	}

}
// namespace AqualinkAutomate::HTTP
