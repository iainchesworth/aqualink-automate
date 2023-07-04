#include <crow/mustache.h>

#include "http/webroute_page_equipment.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Equipment::WebRoute_Page_Equipment(crow::SimpleApp& app, const Kernel::DataHub& data_hub) :
		Interfaces::IWebRoute<PAGE_EQUIPMENT_ROUTE_URL>(app, { { crow::HTTPMethod::Get, std::bind(&WebRoute_Page_Equipment::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Page_Equipment::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
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
