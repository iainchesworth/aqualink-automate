#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "jandy/utility/screen_data_page.h"

namespace AqualinkAutomate::Utility
{
	enum class ScreenDataPageTypes
	{
		Page_Home,
		Page_Service,
		Page_TimeOut,
		Page_OneTouch,
		Page_System,
		Page_EquipmentStatus,
		Page_SelectSpeed,
		Page_MenuHelp,
		Page_SetPoolHeat,
		Page_SetSpaHeat,
		Page_SetTemperature,
		Page_SetTime,
		Page_SystemSetup,
		Page_FreezeProtect,
		Page_Boost,
		Page_SetAquapure,
		Page_Version,
		Page_DiagnosticsSensors,
		Page_DiagnosticsRemotes,
		Page_DiagnosticsErrors,
		Page_Unknown
	};

	class ScreenDataPage_Processor
	{
	public:
		using MenuMatcherDetails = std::pair<uint8_t, std::string>;
		using MenuMatcherProcessor = std::function<void(const ScreenDataPage&)>;

	public:
		ScreenDataPage_Processor(ScreenDataPageTypes page_type, const MenuMatcherDetails menu_matcher, MenuMatcherProcessor menu_processor) :
			m_PageType(page_type),
			m_MenuMatcher(menu_matcher),
			m_MenuProcessor(menu_processor)
		{
		}

	public:
		bool CanProcess(const ScreenDataPage& page) const
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

		void Process(const ScreenDataPage& page)
		{
			m_MenuProcessor(page);
		}

	private:
		const ScreenDataPageTypes m_PageType;
		const MenuMatcherDetails m_MenuMatcher;
		MenuMatcherProcessor m_MenuProcessor;
	};

}
// namespace AqualinkAutomate::Devices
