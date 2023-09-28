#include "http/json/json_equipment.h"
#include "http/webroute_equipment_devices.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Devices::WebRoute_Equipment_Devices(Kernel::HubLocator& hub_locator) :
		Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
	}
	
    HTTP::Message WebRoute_Equipment_Devices::OnRequest(HTTP::Request req)
    {
        HTTP::Response resp{HTTP::Status::ok, req.version()};

        resp.set(boost::beast::http::field::server, "1.2.3.4");
        resp.set(boost::beast::http::field::content_type, "application/json");
        resp.keep_alive(req.keep_alive());
        resp.body() = JSON::GenerateJson_Equipment_Devices(m_DataHub).dump();
        resp.prepare_payload();

        return resp;
	}

}
// namespace AqualinkAutomate::HTTP
