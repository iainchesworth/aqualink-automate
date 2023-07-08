#include <bustache/render/string.hpp>

#include "http/webroute_page_index.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"
#include "jandy/formatters/temperature_formatter.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Index::WebRoute_Page_Index(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebPageRoute<PAGE_INDEX_ROUTE_URL, PAGE_INDEX_TEMPLATE>(http_server),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Page_Index::WebRequestHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		auto template_page = LoadTemplateFromFile(PAGE_INDEX_TEMPLATE);
		auto parsed_template = bustache::format(template_page);

		BustacheTemplateValues template_values;

		Support::GeneratePageHeader_Context(template_values);

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			template_values.emplace("pool_temperature", "-");
			template_values.emplace("spa_temperature", "-");
			template_values.emplace("air_temperature", "-");

			template_values.emplace("water_orp", "-");
			template_values.emplace("water_ph", "-");
		}
		else
		{
			template_values.emplace("pool_temperature", std::format("{}", m_DataHub.PoolTemp()));
			template_values.emplace("spa_temperature", std::format("{}", m_DataHub.SpaTemp()));
			template_values.emplace("air_temperature", std::format("{}", m_DataHub.AirTemp()));

			template_values.emplace("water_orp", "-");
			template_values.emplace("water_ph", "-");
		}		

		Support::GeneratePageFooter_Context(template_values);

		std::string generated_page = bustache::to_string(parsed_template(template_values).context(bustache::no_context_t{}).escape(bustache::escape_html));

		resp.set_status_and_content(
			cinatra::status_type::ok,
			std::move(generated_page),
			cinatra::req_content_type::html,
			cinatra::content_encoding::none
		);
	}

}
// namespace AqualinkAutomate::HTTP
