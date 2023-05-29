#pragma once

#include <cstdint>
#include <list>

#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_processor.h"
#include "jandy/utility/screen_data_page_updater.h"

namespace AqualinkAutomate::Devices
{

	enum class JandyScreenModes
	{
		Normal,
		Updating,
		UpdateComplete
	};

	class JandyScreen
	{
	public:
		JandyScreen(uint8_t screen_lines);

	public:
		Utility::ScreenDataPage const& DisplayedPage() const;

	private:
		Utility::ScreenDataPage m_DisplayedPage;

	public:
		template<typename EVENT_TYPE>
		void ProcessScreenEvent(const EVENT_TYPE& event_type)
		{
			m_DisplayedPageUpdater.process_event(event_type);
		}

	public:
		void ProcessScreenUpdates();

	public:
		JandyScreenModes ScreenMode() const;
		void ScreenMode(JandyScreenModes screen_mode);

	private:
		JandyScreenModes m_ScreenMode{ JandyScreenModes::Normal };

	public:
		std::list<Utility::ScreenDataPage_Processor> const& PageProcessors() const;
		void PageProcessors(std::list<Utility::ScreenDataPage_Processor>&& page_processors);

	private:
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage> m_DisplayedPageUpdater;
		std::list<Utility::ScreenDataPage_Processor> m_DisplayedPageProcessors;
	};

}
// namespace AqualinkAutomate::Devices
