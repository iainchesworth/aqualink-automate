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
		Page_EquipmentOnOff,
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
		ScreenDataPage_Processor(ScreenDataPageTypes page_type, const MenuMatcherDetails menu_matcher, MenuMatcherProcessor menu_processor);

	public:
		ScreenDataPageTypes PageType() const;

	public:
		bool CanProcess(const ScreenDataPage& page) const;
		void Process(const ScreenDataPage& page) const;

	private:
		const ScreenDataPageTypes m_PageType;
		const MenuMatcherDetails m_MenuMatcher;
		MenuMatcherProcessor m_MenuProcessor;
	};

}
// namespace AqualinkAutomate::Devices
