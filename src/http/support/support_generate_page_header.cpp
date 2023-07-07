#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP::Support
{

	void GeneratePageHeader_Context(std::unordered_map<std::string, std::string>& template_value_map)
	{
		template_value_map.emplace("controller_type", "ABCD");
		template_value_map.emplace("controller_firmware", "1.2.3.4");
		template_value_map.emplace("controller_date", "1/1/2000");
		template_value_map.emplace("controller_time", "12:00:00 AM");
	}

}
// namespace AqualinkAutomate::HTTP::Support
