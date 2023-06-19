#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP::Support
{

	void GeneratePageHeader_Context(crow::mustache::context& ctx)
	{
		ctx["controller_type"] = "ABCD";
		ctx["controller_firmware"] = "1.2.3.4";
		ctx["controller_date"] = "1/1/2000";
		ctx["controller_time"] = "12:00:00 AM";
	}

}
// namespace AqualinkAutomate::HTTP::Support
