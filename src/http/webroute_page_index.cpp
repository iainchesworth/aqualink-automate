#include <mstch/mstch.hpp>

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

		mstch::map template_map
		{
		};

		Support::GeneratePageHeader_Context(template_map);

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			template_map.emplace("pool_temperature", std::string{"-"});
			template_map.emplace("spa_temperature", std::string{"-"});
			template_map.emplace("air_temperature", std::string{"-"});

			template_map.emplace("water_orp", std::string{"-"});
			template_map.emplace("water_ph", std::string{"-"});
		}
		else
		{
			template_map.emplace("pool_temperature", std::format("{}", m_DataHub.PoolTemp()));
			template_map.emplace("spa_temperature", std::format("{}", m_DataHub.SpaTemp()));
			template_map.emplace("air_temperature", std::format("{}", m_DataHub.AirTemp()));

			template_map.emplace("water_orp", std::string{"-"});
			template_map.emplace("water_ph", std::string{"-"});
		}

		Support::GeneratePageFooter_Context(template_map);

		std::string generated_page = mstch::render(template_page, template_map);

		resp.set_status_and_content(
			cinatra::status_type::ok,
			std::move(generated_page),
			cinatra::req_content_type::html,
			cinatra::content_encoding::none
		);
	}

}
// namespace AqualinkAutomate::HTTP
