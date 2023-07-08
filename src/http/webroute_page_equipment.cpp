#include <bustache/render/string.hpp>

#include "http/webroute_page_equipment.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Equipment::WebRoute_Page_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebPageRoute<PAGE_EQUIPMENT_ROUTE_URL, PAGE_EQUIPMENT_TEMPLATE>(http_server),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Page_Equipment::WebRequestHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		auto template_page = LoadTemplateFromFile(PAGE_EQUIPMENT_TEMPLATE);
		auto parsed_template = bustache::format(template_page);

		BustacheTemplateValues template_values;
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
