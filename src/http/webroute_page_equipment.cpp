#include <bustache/render/string.hpp>

#include "http/webroute_page_equipment.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Equipment::WebRoute_Page_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebRoute<PAGE_EQUIPMENT_ROUTE_URL>(http_server, { { HTTP::Methods::GET, std::bind(&WebRoute_Page_Equipment::WebRequestHandler, this, std::placeholders::_1, std::placeholders::_2) } }),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Page_Equipment::WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		auto templated_page = ReadTemplateContents("equipment.html.mustache");
		auto parsed_template = bustache::format(templated_page);

		std::unordered_map<std::string, std::string> template_values; 

		Support::GeneratePageHeader_Context(template_values);
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
