#include "http/json/json_equipment.h"
#include "http/webroute_equipment_version.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Version::WebRoute_Equipment_Version(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>(http_server, { { HTTP::Methods::GET, std::bind(&WebRoute_Equipment_Version::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Equipment_Version::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		resp.set_status_and_content(
			cinatra::status_type::ok,
			JSON::GenerateJson_Equipment_Version(m_DataHub).dump(),
			cinatra::req_content_type::json,
			cinatra::content_encoding::none
		);
	}

}
// namespace AqualinkAutomate::HTTP
