#include <crow/mustache.h>

#include "http/webroute_page_version.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Version::WebRoute_Page_Version(crow::SimpleApp& app) :
		Interfaces::IWebRoute<PAGE_VERSION_ROUTE_URL>(app)
	{
	}

	void WebRoute_Page_Version::WebRequestHandler(const Request& req, Response& resp)
	{
		crow::mustache::context ctx;
		//crow::mustache::set_base(m_DocRoot);

		auto page = crow::mustache::load("version.html.mustache");

		Support::GeneratePageHeader_Context(ctx);
		Support::GeneratePageFooter_Context(ctx);

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
