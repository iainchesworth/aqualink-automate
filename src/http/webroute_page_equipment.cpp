#include <mstch/mstch.hpp>

#include "http/webroute_page_equipment.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Equipment::WebRoute_Page_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebPageRoute<PAGE_EQUIPMENT_ROUTE_URL, PAGE_EQUIPMENT_TEMPLATE>(http_server),
		Interfaces::IShareableRoute(),
		m_DataHub(data_hub)
	{
	}

	void WebRoute_Page_Equipment::WebRequestHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		Support::GeneratePageHeader_Context(m_TemplateContext);
		Support::GeneratePageFooter_Context(m_TemplateContext);

		resp.set_status_and_content(
			cinatra::status_type::ok,
			mstch::render(m_TemplateContent, m_TemplateContext),
			cinatra::req_content_type::html,
			cinatra::content_encoding::none
		);
	}

}
// namespace AqualinkAutomate::HTTP
