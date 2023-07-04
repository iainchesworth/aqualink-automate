#include "http/json/json_equipment.h"
#include "http/webroute_equipment_version.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Version::WebRoute_Equipment_Version(crow::SimpleApp& app, const Kernel::DataHub& data_hub) :
		Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>(app, { { crow::HTTPMethod::Get, std::bind(&WebRoute_Equipment_Version::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		m_DataHub(data_hub)
	{
	}
	
	void WebRoute_Equipment_Version::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		resp.set_header("Content-Type", "application/json");
		resp.body = JSON::GenerateJson_Equipment_Version(m_DataHub).dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
