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

	std::string WebRoute_Page_Version::GenerateBody(HTTP::Request req)
	{
		Support::GeneratePageHeader_Context(m_TemplateContext);
		Support::GeneratePageFooter_Context(m_TemplateContext);

		return mstch::render(m_TemplateContent, m_TemplateContext);
	}

}
// namespace AqualinkAutomate::HTTP
