#include <crow/mustache.h>

#include "http/webroute_page_version.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Version::WebRoute_Page_Version(crow::SimpleApp& app) :
		Interfaces::IWebRoute<PAGE_VERSION_ROUTE_URL>(app, { { crow::HTTPMethod::Get, std::bind(&WebRoute_Page_Version::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } })
	{
	}

	void WebRoute_Page_Version::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		crow::mustache::context ctx;

		auto page = crow::mustache::load("version.html.mustache");

		Support::GeneratePageHeader_Context(ctx);
		Support::GeneratePageFooter_Context(ctx);

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
