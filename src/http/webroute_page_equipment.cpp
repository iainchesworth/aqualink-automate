#include <mstch/mstch.hpp>

#include "http/webroute_page_equipment.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Page_Equipment::WebRoute_Page_Equipment(Kernel::HubLocator& hub_locator)
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
	}

	std::string WebRoute_Page_Equipment::GenerateBody(HTTP::Request req)
	{
		Support::GeneratePageHeader_Context(m_TemplateContext);
		Support::GeneratePageFooter_Context(m_TemplateContext);

		return mstch::render(m_TemplateContent.value(), m_TemplateContext);
	}

}
// namespace AqualinkAutomate::HTTP
