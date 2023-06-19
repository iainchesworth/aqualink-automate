#include <format>
#include <ranges>

#include <magic_enum.hpp>

#include "jandy/devices/capabilities/screen.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices::Capabilities
{

	Screen::Screen(uint8_t screen_lines) :
		m_DisplayedPage(screen_lines),
		m_DisplayedPageType(Utility::ScreenDataPageTypes::Page_Unknown),
		m_DisplayedPageUpdater(m_DisplayedPage),
		m_DisplayedPageProcessors()
	{
		m_DisplayedPageUpdater.initiate();
	}

	void Screen::ProcessScreenUpdates()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Screen -> Processing Screen Updates", std::source_location::current());

		switch (m_ScreenMode)
		{
		case ScreenModes::Normal:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Screen -> Processing Screen Updates (Normal Mode)", std::source_location::current());
			LogTrace(Channel::Devices, "Device screen mode -> normal");
			break;
		}

		case ScreenModes::Updating:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Screen -> Processing Screen Updates (Updating Mode)", std::source_location::current());
			LogTrace(Channel::Devices, "Device screen mode -> updating");
			break;
		}

		case ScreenModes::UpdateComplete:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Screen -> Processing Screen Updates (Update Complete Mode)", std::source_location::current());
			LogTrace(Channel::Devices, "Device screen mode -> update complete");

			// Set the current page to "unknown"; if there's a page processor, we'll set the page to that later...
			m_DisplayedPageType = Utility::ScreenDataPageTypes::Page_Unknown;

			// Process the "page" to extract information.
			auto actionable_processors = m_DisplayedPageProcessors | std::views::filter([this](const decltype(m_DisplayedPageProcessors)::value_type& processor) { return processor.CanProcess(m_DisplayedPage); });
			for (auto& processor : actionable_processors)
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Screen -> Processing Screen Updates -> Processing Page", std::source_location::current());
				LogTrace(Channel::Devices, std::format("Device screen mode -> processing page {}", magic_enum::enum_name(processor.PageType())));

				// As there's a specific processor, set the page type to the processor's page type.
				m_DisplayedPageType = processor.PageType();

				// Process the page.
				processor.Process(m_DisplayedPage);
			}

			m_ScreenMode = ScreenModes::Normal;
			break;
		}
		}
	}

	ScreenModes Screen::ScreenMode() const
	{
		return m_ScreenMode;
	}

	void Screen::ScreenMode(ScreenModes screen_mode)
	{
		m_ScreenMode = screen_mode;
	}

	std::list<Utility::ScreenDataPage_Processor> const& Screen::PageProcessors() const
	{
		return m_DisplayedPageProcessors;
	}

	void Screen::PageProcessors(std::list<Utility::ScreenDataPage_Processor>&& page_processors)
	{
		m_DisplayedPageProcessors = std::move(page_processors);
	}

	Utility::ScreenDataPage const& Screen::DisplayedPage() const
	{
		return m_DisplayedPage;
	}

	Utility::ScreenDataPageTypes Screen::DisplayedPageType() const
	{
		return m_DisplayedPageType;
	}

}
// namespace AqualinkAutomate::Devices::Capabilities
