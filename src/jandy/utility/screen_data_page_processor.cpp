#include <format>

#include "jandy/utility/screen_data_page_processor.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	ScreenDataPage_Processor::ScreenDataPage_Processor(ScreenDataPageTypes page_type, const MenuMatcherDetails menu_matcher, MenuMatcherProcessor menu_processor) :
		m_PageType(page_type),
		m_MenuMatcher(menu_matcher),
		m_MenuProcessor(menu_processor)
	{
	}

	ScreenDataPageTypes ScreenDataPage_Processor::PageType() const
	{
		return m_PageType;
	}

	bool ScreenDataPage_Processor::CanProcess(const ScreenDataPage& page) const
	{
		if (m_MenuMatcher.first >= page.Size())
		{
			LogDebug(Channel::Devices, std::format("Attempted to process a line on the page which doesn't exist; requested line -> {} (0-index), total lines -> {}", m_MenuMatcher.first, page.Size()));
		}
		else
		{
			auto& haystack = page[m_MenuMatcher.first].Text;
			auto& needle = m_MenuMatcher.second;

			auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), [](unsigned char ch1, unsigned char ch2)
				{
					return std::toupper(ch1) == std::toupper(ch2);
				}
			);

			return (it != haystack.end());
		}

		return false;
	}

	void ScreenDataPage_Processor::Process(const ScreenDataPage& page) const
	{
		m_MenuProcessor(page);
	}

}
// namespace AqualinkAutomate::Devices
