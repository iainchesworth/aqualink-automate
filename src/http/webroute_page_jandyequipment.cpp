#include <crow/mustache.h>

#include "http/webroute_page_jandyequipment.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_JandyEquipment::WebRoute_Page_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<PAGE_EQUIPMENT_ROUTE_URL>(app),
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebRoute_Page_JandyEquipment::WebRequestHandler(const Request& req, Response& resp)
	{
		crow::mustache::context ctx;

		auto page = crow::mustache::load("equipment.html.mustache");

		Support::GeneratePageHeader_Context(ctx);
		Support::GeneratePageFooter_Context(ctx);

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
