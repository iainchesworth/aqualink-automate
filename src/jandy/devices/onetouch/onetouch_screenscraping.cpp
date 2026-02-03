#include <list>
#include <tuple>

#include "devices/device_status.h"
#include "devices/onetouch_device.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{
	const OneTouchDevice::Scrapeable::ScraperGraph OneTouchDevice::ONETOUCH_AUX_LABELS_NAV_SCRAPER_GRAPH
	{
		{
			{ 1, Utility::ScreenDataPageTypes::Page_OneTouch },				// ONETOUCH -> (line down) -> ONETOUCH
			{ 2, Utility::ScreenDataPageTypes::Page_OneTouch },				// ONETOUCH -> (select) -> HOME
			{ 3, Utility::ScreenDataPageTypes::Page_Home },					// HOME -> (line up) -> HOME
			{ 4, Utility::ScreenDataPageTypes::Page_Home },					// HOME -> (select) -> MENU
			{ 5, Utility::ScreenDataPageTypes::Page_MenuHelp },				// MENU -> (line down) -> MENU
			{ 6, Utility::ScreenDataPageTypes::Page_MenuHelp },				// MENU -> (line down) -> MENU
			{ 7, Utility::ScreenDataPageTypes::Page_MenuHelp },				// MENU -> (line down) -> MENU
			{ 8, Utility::ScreenDataPageTypes::Page_MenuHelp },				// MENU -> (line down) -> MENU
			{ 9, Utility::ScreenDataPageTypes::Page_MenuHelp },				// MENU -> (select) -> SYSTEM SETUP
			{ 10, Utility::ScreenDataPageTypes::Page_SystemSetup },			// SYSTEM SETUP -> (line down) -> SYSTEM SETUP
			{ 11, Utility::ScreenDataPageTypes::Page_SystemSetup },			// SYSTEM SETUP -> (select) -> LABEL AUX LIST
			{ 12, Utility::ScreenDataPageTypes::Page_Unknown }				// ...end
		},
		{
			{ 1, 2, { 1, KeyCommands::LineDown }},
			{ 2, 3, { 2, KeyCommands::Select }},
			{ 3, 4, { 3, KeyCommands::LineUp }},
			{ 4, 5, { 4, KeyCommands::Select }},
			{ 5, 6, { 5, KeyCommands::LineDown }},
			{ 6, 7, { 6, KeyCommands::LineDown }},
			{ 7, 8, { 7, KeyCommands::LineDown }},
			{ 8, 9, { 8, KeyCommands::LineDown }},
			{ 9, 10, { 9, KeyCommands::Select }},
			{ 10, 11, { 10, KeyCommands::LineDown }},
			{ 11, 12, { 11, KeyCommands::Select }}
		}
	};

	const OneTouchDevice::Scrapeable::ScraperGraph OneTouchDevice::ONETOUCH_AUX_LABELS_TEXT_SCRAPER_GRAPH
	{
		{
			{ 1, Utility::ScreenDataPageTypes::Page_LabelAuxList },			// LABEL AUX LIST -> (select) -> LABEL AUX
			{ 2, Utility::ScreenDataPageTypes::Page_LabelAux },				// LABEL AUX -> (back) -> LABEL AUX LIST
			{ 3, Utility::ScreenDataPageTypes::Page_LabelAuxList },			// LABEL AUX LIST -> (line down) -> LABEL AUX LIST
			{ 4, Utility::ScreenDataPageTypes::Page_Unknown }				// ...end
		},
		{
			{ 1, 2, { 1, KeyCommands::Select }},
			{ 2, 3, { 2, KeyCommands::Back_Or_Select2 }},
			{ 3, 4, { 3, KeyCommands::LineDown }}
		}
	};

	const std::list<OneTouchDevice::Scrapeable::ScrapeId> OneTouchDevice::STARTUP_SCRAPE_GRAPHS
	{
		ONETOUCH_AUX_LABELS_NAV_SCRAPER,

		// AUX 1-7
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,

		// AUX B1-B8
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,
		ONETOUCH_AUX_LABELS_TEXT_SCRAPER,

		ONETOUCH_CONFIG_INIT_SCRAPER
	};

	const OneTouchDevice::Scrapeable::ScraperGraph OneTouchDevice::ONETOUCH_CONFIG_INIT_SCRAPER_GRAPH
	{
		{
			{ 1, Utility::ScreenDataPageTypes::Page_SystemSetup },			// LABEL AUX LIST -> (back) -> SYSTEM SETUP
			{ 2, Utility::ScreenDataPageTypes::Page_MenuHelp },				// SYSTEM SETUP -> (back) -> MENU
			{ 3, Utility::ScreenDataPageTypes::Page_Home },					// MENU -> (back) -> HOME
			{ 4, Utility::ScreenDataPageTypes::Page_EquipmentOnOff },		// HOME -> (select) -> EQUIPMENT STATUS
			{ 5, Utility::ScreenDataPageTypes::Page_EquipmentOnOff },		// EQUIPMENT STATUS -> (page down) -> EQUIPMENT STATUS
			{ 6, Utility::ScreenDataPageTypes::Page_EquipmentOnOff },		// EQUIPMENT STATUS -> (page down) -> EQUIPMENT STATUS
			{ 7, Utility::ScreenDataPageTypes::Page_Home },					// EQUIPMENT STATUS -> (back) -> HOME
			{ 8, Utility::ScreenDataPageTypes::Page_Home },					// HOME -> (line down) -> HOME
			{ 9, Utility::ScreenDataPageTypes::Page_Home },					// HOME -> (line down) -> HOME
			{ 10, Utility::ScreenDataPageTypes::Page_MenuHelp },			// HOME -> (select) -> MENU
			{ 11, Utility::ScreenDataPageTypes::Page_MenuHelp },			// MENU -> (select) -> HELP
			{ 12, Utility::ScreenDataPageTypes::Page_MenuHelp },			// HELP -> (line down) -> HELP
			{ 13, Utility::ScreenDataPageTypes::Page_MenuHelp },			// HELP -> (line down) -> HELP
			{ 14, Utility::ScreenDataPageTypes::Page_DiagnosticsSensors },	// HELP -> (select) -> DIAGNOSTICS
			{ 15, Utility::ScreenDataPageTypes::Page_DiagnosticsRemotes },	// DIAGNOSTICS -> (select) -> DIAGNOSTICS
			{ 16, Utility::ScreenDataPageTypes::Page_DiagnosticsErrors },	// DIAGNOSTICS -> (select) -> DIAGNOSTICS 
			{ 17, Utility::ScreenDataPageTypes::Page_MenuHelp },			// DIAGNOSTICS -> (select) -> HELP
			{ 18, Utility::ScreenDataPageTypes::Page_MenuHelp },			// HELP -> (back) -> MENU
			{ 19, Utility::ScreenDataPageTypes::Page_Home },				// MENU -> (back) -> HOME
			{ 20, Utility::ScreenDataPageTypes::Page_Unknown }				// ...end
		},
		{
			{ 1, 2, { 1, KeyCommands::Back_Or_Select2 }},
			{ 2, 3, { 2, KeyCommands::Back_Or_Select2 }},
			{ 3, 4, { 3, KeyCommands::Back_Or_Select2 }},
			{ 4, 5, { 4, KeyCommands::Select }},
			{ 5, 6, { 5, KeyCommands::PageDown_Or_Select1 }},
			{ 6, 7, { 6, KeyCommands::PageDown_Or_Select1 }},
			{ 7, 8, { 7, KeyCommands::Back_Or_Select2 }},
			{ 8, 9, { 8, KeyCommands::LineDown }},
			{ 9, 10, { 9, KeyCommands::LineDown }},
			{ 10, 11, { 10, KeyCommands::Select }},
			{ 11, 12, { 11, KeyCommands::Select }},
			{ 12, 13, { 12, KeyCommands::LineDown }},
			{ 13, 14, { 13, KeyCommands::LineDown }},
			{ 14, 15, { 14, KeyCommands::Select }},
			{ 15, 16, { 15, KeyCommands::Select }},
			{ 16, 17, { 16, KeyCommands::Select }},
			{ 17, 18, { 17, KeyCommands::Select }},
			{ 18, 19, { 18, KeyCommands::Back_Or_Select2 }},
			{ 19, 20, { 19, KeyCommands::Back_Or_Select2 }}
		}
	};


	void OneTouchDevice::Scraping_ProcessStep_StartUp()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::Scraping_ProcessStep_StartUp", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): Processing StartUp scraping step", DeviceId()));

		Status(Devices::DeviceStatus_Initializing{});

		switch (DisplayedPageType())
		{
		case Utility::ScreenDataPageTypes::Page_OneTouch:
			LogInfo(Channel::Devices, std::format("OneTouch({}) : Initiating COLD START: detected OneTouch page", DeviceId()));
			m_OpState = OperatingStates::ColdStart;
			ScrapingStart(ONETOUCH_AUX_LABELS_NAV_SCRAPER, ONETOUCH_COLD_START_SCRAPER_START_INDEX);
			LogDebug(Channel::Devices, std::format("OneTouch({}) : Started scraping: scraper_id={}, start_index={}", DeviceId(), ONETOUCH_AUX_LABELS_NAV_SCRAPER, ONETOUCH_COLD_START_SCRAPER_START_INDEX));
			break;

		case Utility::ScreenDataPageTypes::Page_Home:
			LogInfo(Channel::Devices, "Initiating WARM START: detected Home page");
			m_OpState = OperatingStates::WarmStart;
			ScrapingStart(ONETOUCH_AUX_LABELS_NAV_SCRAPER, ONETOUCH_WARM_START_SCRAPER_START_INDEX);
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Started scraping: scraper_id={}, start_index={}", DeviceId(), ONETOUCH_AUX_LABELS_NAV_SCRAPER, ONETOUCH_WARM_START_SCRAPER_START_INDEX));
			break;

		default:
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StartUp waiting for valid page: current_page={}", DeviceId(), magic_enum::enum_name(DisplayedPageType())));
			break;
		}
	}

	void OneTouchDevice::Scraping_ProcessStep_ColdAndWarmStart()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::Scraping_ProcessStep_ColdAndWarmStart", std::source_location::current());

		LogTrace(Channel::Devices, std::format("OneTouch ({}): Processing {}", DeviceId(), magic_enum::enum_name(m_OpState)));

		auto step_through_start_up_scrape_graphs = [this]() -> void
		{
			// Iterate along the list of graphs (noting that the first element was actioned in the StartUp state.
			if (STARTUP_SCRAPE_GRAPHS.cend() == m_StartUpScrapeGraphsIt)
			{
				// Unexpectedly reached the end of the scrape graph...log an error.
				LogWarning(Channel::Devices, std::format("OneTouch ({}): Unexpectedly reached the end of sequence for emulated OneTouch device initialisation.", DeviceId()));
			}
			else if (++m_StartUpScrapeGraphsIt; STARTUP_SCRAPE_GRAPHS.cend() == m_StartUpScrapeGraphsIt)
			{
				// NOTE: Flow was VERSION -> ONETOUCH/HOME -> [scraping] -> HOME
				LogInfo(Channel::Devices, std::format("OneTouch ({}): Emulated OneTouch device initialisation ({}) complete -> entering normal operation", DeviceId(), (OperatingStates::ColdStart == m_OpState) ? "COLD START" : "WARM START"));
				m_OpState = OperatingStates::NormalOperation;
				Status(Devices::DeviceStatus_Normal{});
				LogDebug(Channel::Devices, std::format("OneTouch ({}): Configuration scraping completed successfully", DeviceId()));
			}
			else
			{
				const auto graph_position = std::distance(STARTUP_SCRAPE_GRAPHS.cbegin(), m_StartUpScrapeGraphsIt);
				LogDebug(Channel::Devices, std::format("OneTouch ({}): Starting next scrape graph: scraper_id={}, position={}/{}", DeviceId(), *m_StartUpScrapeGraphsIt, graph_position, STARTUP_SCRAPE_GRAPHS.size()));
				ScrapingStart(*m_StartUpScrapeGraphsIt, 1);
			}
		};

		auto scrape_step_outcome = ScrapingNext();
		if (!scrape_step_outcome.has_value())
		{
			switch (scrape_step_outcome.error())
			{
			case ErrorCodes::Scrapeable_ErrorCodes::WaitingForPage:
				LogTrace(Channel::Devices, std::format("OneTouch ({}): Emulated OneTouch device: scrape in-progress -> waiting on page", DeviceId()));
				++m_ScrapingStallCounter;
				break;

			case ErrorCodes::Scrapeable_ErrorCodes::WaitingForMessage:
				LogTrace(Channel::Devices, std::format("OneTouch ({}): Emulated OneTouch device: scrape in-progress -> waiting for message", DeviceId()));
				++m_ScrapingStallCounter;
				break;

			case ErrorCodes::Scrapeable_ErrorCodes::NoStepPossible:
				LogDebug(Channel::Devices, std::format("OneTouch ({}): Current scrape graph complete, moving to next", DeviceId()));
				m_ScrapingStallCounter = 0;
				step_through_start_up_scrape_graphs();
				break;

			case ErrorCodes::Scrapeable_ErrorCodes::NoGraphBeingScraped:
				LogWarning(Channel::Devices, std::format("OneTouch ({}): No active scrape during {} - forcing normal operation", DeviceId(), m_OpState == OperatingStates::ColdStart ? "ColdStart" : "WarmStart"));
				m_ScrapingStallCounter = 0;
				m_OpState = OperatingStates::NormalOperation;
				Status(Devices::DeviceStatus_Normal{});
				break;

			case ErrorCodes::Scrapeable_ErrorCodes::UnknownScrapeError:
				[[fallthrough]];
			default:
				// No scrape is active (or waiting) but it's a cold start...this is weird so force a transition to normal operation.
				LogWarning(Channel::Devices, std::format("OneTouch ({}): Emulated OneTouch device initialisation ({}) in an abnormal state -> forcing entry to normal operation", DeviceId(), (OperatingStates::ColdStart == m_OpState) ? "COLD START" : "WARM START"));
				m_ScrapingStallCounter = 0;
				m_OpState = OperatingStates::NormalOperation;
				Status(Devices::DeviceStatus_Normal{});
				break;
			}

			if (m_ScrapingStallCounter >= ONETOUCH_SCRAPING_STALL_LIMIT)
			{
				LogWarning(Channel::Devices, std::format("OneTouch ({}): Scraping stalled for {} iterations during {} -> abandoning scraping and entering NormalOperation", DeviceId(), m_ScrapingStallCounter, magic_enum::enum_name(m_OpState)));
				m_ScrapingStallCounter = 0;
				m_OpState = OperatingStates::NormalOperation;
				Status(Devices::DeviceStatus_Normal{});
			}
		}
		else
		{
			m_ScrapingStallCounter = 0;

			try
			{
				m_KeyCommand_ToSend = std::any_cast<KeyCommands>(scrape_step_outcome.value());
				LogDebug(Channel::Devices, std::format("OneTouch ({}): Emulated OneTouch device: scrape in-progress - sending next command: {}", DeviceId(), magic_enum::enum_name(m_KeyCommand_ToSend)));
			}
			catch (const std::bad_any_cast& eAC)
			{
				LogWarning(Channel::Devices, std::format("OneTouch ({}): Failed trying to get key command for next step: error -> {}", DeviceId(), eAC.what()));
			}
		}
	}

}
// namespace AqualinkAutomate::Devices
