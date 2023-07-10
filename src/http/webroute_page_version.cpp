#include <mstch/mstch.hpp>

#include "http/webroute_page_version.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Version::WebRoute_Page_Version(HTTP::Server& http_server) :
		Interfaces::IWebPageRoute<PAGE_VERSION_ROUTE_URL, PAGE_VERSION_TEMPLATE>(http_server)
	{
	}

	void WebRoute_Page_Version::WebRequestHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		auto template_page = LoadTemplateFromFile(PAGE_VERSION_TEMPLATE);
		
		mstch::map template_map
		{
		};

		Support::GeneratePageHeader_Context(template_map);
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
