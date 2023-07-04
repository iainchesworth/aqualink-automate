#include <crow/mustache.h>

#include "http/webroute_page_index.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"
#include "jandy/formatters/temperature_formatter.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Index::WebRoute_Page_Index(crow::SimpleApp& app, const Kernel::DataHub& data_hub) :
		Interfaces::IWebRoute<PAGE_INDEX_ROUTE_URL>(app, { { crow::HTTPMethod::Get, std::bind(&WebRoute_Page_Index::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Page_Index::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		crow::mustache::context ctx;

		auto page = crow::mustache::load("index.html.mustache");

		Support::GeneratePageHeader_Context(ctx);

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			ctx["pool_temperature"] = "-";
			ctx["spa_temperature"] = "-";
			ctx["air_temperature"] = "-";

			ctx["water_orp"] = "-";
			ctx["water_ph"] = "-";
		}
		else
		{
			ctx["pool_temperature"] = std::format("{}", m_DataHub.PoolTemp());
			ctx["spa_temperature"] = std::format("{}", m_DataHub.SpaTemp());
			ctx["air_temperature"] = std::format("{}", m_DataHub.AirTemp());

			ctx["water_orp"] = "-";
			ctx["water_ph"] = "-";
		}		

		Support::GeneratePageFooter_Context(ctx);

		resp.set_header("Content-Type", "text/html");
		resp.body = page.render_string(ctx);
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
