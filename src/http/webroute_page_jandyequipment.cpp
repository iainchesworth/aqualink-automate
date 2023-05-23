#include <crow/mustache.h>

#include "http/webroute_page_jandyequipment.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_JandyEquipment::WebRoute_Page_JandyEquipment(crow::SimpleApp& app, const std::string& doc_root, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<PAGE_EQUIPMENT_ROUTE_URL>(app, doc_root),
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebRoute_Page_JandyEquipment::WebRequestHandler(const Request& req, Response& resp)
	{
		crow::mustache::context ctx;
		crow::mustache::set_base(m_DocRoot);

		auto page = crow::mustache::load("equipment.html.mustache");

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
