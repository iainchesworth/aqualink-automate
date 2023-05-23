#include <crow/mustache.h>

#include "http/webroute_page_index.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Index::WebRoute_Page_Index(crow::SimpleApp& app, const std::string& doc_root) :
		Interfaces::IWebRoute<PAGE_INDEX_ROUTE_URL>(app, doc_root)
	{
	}

	void WebRoute_Page_Index::WebRequestHandler(const Request& req, Response& resp)
	{
		crow::mustache::context ctx;
		crow::mustache::set_base(m_DocRoot);

		auto page = crow::mustache::load("index.html.mustache");

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
