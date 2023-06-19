#include <crow/mustache.h>

#include "http/webroute_page_index.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Index::WebRoute_Page_Index(crow::SimpleApp& app) :
		Interfaces::IWebRoute<PAGE_INDEX_ROUTE_URL>(app)
	{
	}

	void WebRoute_Page_Index::WebRequestHandler(const Request& req, Response& resp)
	{
		crow::mustache::context ctx;

		auto page = crow::mustache::load("index.html.mustache");

		Support::GeneratePageHeader_Context(ctx);
		Support::GeneratePageFooter_Context(ctx);

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
