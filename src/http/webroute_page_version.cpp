#include <mstch/mstch.hpp>

#include "http/webroute_page_version.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Version::WebRoute_Page_Version() :
		Interfaces::IWebPageRoute<PAGE_VERSION_ROUTE_URL, PAGE_VERSION_TEMPLATE>()
	{
	}

	HTTP::Message WebRoute_Page_Version::OnRequest(HTTP::Request req)
	{
		Support::GeneratePageHeader_Context(m_TemplateContext);
        Support::GeneratePageFooter_Context(m_TemplateContext);

        HTTP::Response resp{HTTP::Status::ok, req.version()};

        resp.set(boost::beast::http::field::server, "1.2.3.4");
        resp.set(boost::beast::http::field::content_type, "application/json");
        resp.keep_alive(req.keep_alive());
        resp.body() = mstch::render(m_TemplateContent, m_TemplateContext);
        resp.prepare_payload();

        return resp;
	}

}
// namespace AqualinkAutomate::HTTP
