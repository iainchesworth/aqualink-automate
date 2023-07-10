#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP::Support
{

	void GeneratePageHeader_Context(mstch::map& template_value_map)
	{
		template_value_map.emplace("controller_type", std::string{"ABCD"});
		template_value_map.emplace("controller_firmware", std::string{"1.2.3.4"});
		template_value_map.emplace("controller_date", std::string{"1/1/2000"});
		template_value_map.emplace("controller_time", std::string{"12:00:00 AM"});
	}

}
// namespace AqualinkAutomate::HTTP::Support
