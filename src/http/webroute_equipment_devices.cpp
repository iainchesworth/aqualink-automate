#include "http/json/json_equipment.h"
#include "http/webroute_equipment_devices.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Devices::WebRoute_Equipment_Devices(HTTP::Server& http_server, Kernel::HubLocator& hub_locator) :
		Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>(http_server, { { HTTP::Methods::GET, std::bind(&WebRoute_Equipment_Devices::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		Interfaces::IShareableRoute()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
	}

	void WebRoute_Equipment_Devices::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		resp.set_status_and_content(
			cinatra::status_type::ok,
			JSON::GenerateJson_Equipment_Devices(m_DataHub).dump(),
			cinatra::req_content_type::json,
			cinatra::content_encoding::none
		);
	}

}
// namespace AqualinkAutomate::HTTP
