#include <crow/mustache.h>

#include "http/webroute_page_version.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Version::WebRoute_Page_Version(crow::SimpleApp& app, const std::string& doc_root) :
		Interfaces::IWebRoute<PAGE_VERSION_ROUTE_URL>(app, doc_root)
	{
	}

	void WebRoute_Page_Version::WebRequestHandler(const Request& req, Response& resp)
	{
		crow::mustache::context ctx;
		crow::mustache::set_base(m_DocRoot);

		auto page = crow::mustache::load("version.html.mustache");

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
