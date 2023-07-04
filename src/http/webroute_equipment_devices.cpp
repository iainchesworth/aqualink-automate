#include "http/json/json_equipment.h"
#include "http/webroute_equipment_devices.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Devices::WebRoute_Equipment_Devices(crow::SimpleApp& app, const Kernel::DataHub& data_hub) :
		Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>(app, { { crow::HTTPMethod::Get, std::bind(&WebRoute_Equipment_Devices::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Equipment_Devices::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		resp.set_header("Content-Type", "application/json");
		resp.body = JSON::GenerateJson_Equipment_Devices(m_DataHub).dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
