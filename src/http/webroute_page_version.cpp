#include <mstch/mstch.hpp>

#include "http/webroute_page_version.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Version::WebRoute_Page_Version(HTTP::Server& http_server) :
		Interfaces::IWebPageRoute<PAGE_VERSION_ROUTE_URL, PAGE_VERSION_TEMPLATE>(http_server),
		Interfaces::IShareableRoute()
	{
	}

	void WebRoute_Page_Version::WebRequestHandler(HTTP::Request& req, HTTP::Response& resp)
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
