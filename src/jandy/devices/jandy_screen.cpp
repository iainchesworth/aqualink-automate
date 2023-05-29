#include <ranges>

#include "jandy/devices/jandy_screen.h"

namespace AqualinkAutomate::Devices
{

	JandyScreen::JandyScreen(uint8_t screen_lines) :
		m_DisplayedPage(screen_lines),
		m_DisplayedPageUpdater(m_DisplayedPage),
		m_DisplayedPageProcessors()
	{
		m_DisplayedPageUpdater.initiate();
	}

	void JandyScreen::ProcessScreenUpdates()
	{
		switch (m_ScreenMode)
		{
		case JandyScreenModes::Normal:
			break;

		case JandyScreenModes::Updating:
			break;

		case JandyScreenModes::UpdateComplete:
		{
			// Process the "page" to extract information.
			auto actionable_processors = m_DisplayedPageProcessors | std::views::filter([this](const decltype(m_DisplayedPageProcessors)::value_type& processor) { return processor.CanProcess(m_DisplayedPage); });
			for (auto& processor : actionable_processors)
			{
				processor.Process(m_DisplayedPage);
			}
			m_ScreenMode = JandyScreenModes::Normal;
			break;
		}
		}
	}

	JandyScreenModes JandyScreen::ScreenMode() const
	{
		return m_ScreenMode;
	}

	void JandyScreen::ScreenMode(JandyScreenModes screen_mode)
	{
		m_ScreenMode = screen_mode;
	}

	std::list<Utility::ScreenDataPage_Processor> const& JandyScreen::PageProcessors() const
	{
		return m_DisplayedPageProcessors;
	}

	void JandyScreen::PageProcessors(std::list<Utility::ScreenDataPage_Processor>&& page_processors)
	{
		m_DisplayedPageProcessors = std::move(page_processors);
	}

	Utility::ScreenDataPage const& JandyScreen::DisplayedPage() const
	{
		return m_DisplayedPage;
	}

}
// namespace AqualinkAutomate::Devices
