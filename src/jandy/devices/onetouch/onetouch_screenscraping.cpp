#include <list>
#include <tuple>

#include "devices/device_status.h"
#include "jandy/devices/onetouch_device.h"
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
			{ 5, Utility::ScreenDataPageTypes::Page_MenuHelp },				// MENU -> (line up) -> MENU
			{ 6, Utility::ScreenDataPageTypes::Page_MenuHelp },				// MENU -> (select) -> SYSTEM SETUP
			{ 7, Utility::ScreenDataPageTypes::Page_SystemSetup },			// SYSTEM SETUP -> (line down) -> SYSTEM SETUP
			{ 8, Utility::ScreenDataPageTypes::Page_SystemSetup },			// SYSTEM SETUP -> (select) -> LABEL AUX LIST
			{ 9, Utility::ScreenDataPageTypes::Page_Unknown }				// ...end
		},
		{
			{ 1, 2, { 1, KeyCommands::LineDown }},
			{ 2, 3, { 2, KeyCommands::Select }},
			{ 3, 4, { 3, KeyCommands::LineUp }},
			{ 4, 5, { 4, KeyCommands::Select }},
			{ 5, 6, { 5, KeyCommands::LineUp }},
			{ 6, 7, { 6, KeyCommands::Select }},
			{ 7, 8, { 7, KeyCommands::LineDown }},
			{ 8, 9, { 8, KeyCommands::Select }}
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
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Scraping_ProcessStep_StartUp", BOOST_CURRENT_LOCATION);

		Status(Devices::DeviceStatus_Initializing{});

		switch (DisplayedPageType())
		{
		case Utility::ScreenDataPageTypes::Page_OneTouch:
			LogDebug(Channel::Devices, "Emulated OneTouch device: scrape starting - initialising config (from OneTouch page)");
			m_OpState = OperatingStates::ColdStart;
			ScrapingStart(ONETOUCH_AUX_LABELS_NAV_SCRAPER, ONETOUCH_COLD_START_SCRAPER_START_INDEX);
			break;

		case Utility::ScreenDataPageTypes::Page_Home:
			LogDebug(Channel::Devices, "Emulated OneTouch device: scrape starting - initialising config (from Home page)");
			m_OpState = OperatingStates::WarmStart;
			ScrapingStart(ONETOUCH_AUX_LABELS_NAV_SCRAPER, ONETOUCH_WARM_START_SCRAPER_START_INDEX);
			break;

		default:
			// DO NOTHING HERE
			break;
		}
	}

	void OneTouchDevice::Scraping_ProcessStep_ColdAndWarmStart()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Scraping_ProcessStep_ColdAndWarmStart", BOOST_CURRENT_LOCATION);

		auto step_through_start_up_scrape_graphs = [this]() -> void
		{
			static const std::list<Scrapeable::ScrapeId> start_up_scrape_graphs
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
				ONETOUCH_AUX_LABELS_TEXT_SCRAPER,

				ONETOUCH_CONFIG_INIT_SCRAPER 
			};

			static std::list<Scrapeable::ScrapeId>::const_iterator start_up_scrape_graphs_it{ start_up_scrape_graphs.cbegin() };

			// Iterate along the list of graphs (noting that the first element was actioned in the StartUp state.
			++start_up_scrape_graphs_it;

			if (start_up_scrape_graphs.cend() == start_up_scrape_graphs_it)
			{
				// NOTE: Flow was VERSION -> ONETOUCH/HOME -> [scraping] -> HOME
				LogInfo(Channel::Devices, std::format("Emulated OneTouch device initialisation ({}) complete -> entering normal operation", (OperatingStates::ColdStart == m_OpState) ? "COLD START" : "WARM START"));
				m_OpState = OperatingStates::NormalOperation;
				Status(Devices::DeviceStatus_Normal{});
			}
			else
			{
				ScrapingStart(*start_up_scrape_graphs_it, 1);
			}
		};

		auto scrape_step_outcome = ScrapingNext();
		if (!scrape_step_outcome.has_value())
		{
			switch (scrape_step_outcome.error())
			{
			case ErrorCodes::Scrapeable_ErrorCodes::WaitingForPage:
				LogTrace(Channel::Devices, "Emulated OneTouch device: scrape in-progress -> waiting on page");
				break;

			case ErrorCodes::Scrapeable_ErrorCodes::WaitingForMessage:
				LogTrace(Channel::Devices, "Emulated OneTouch device: scrape in-progress -> waiting for message");
				break;

			case ErrorCodes::Scrapeable_ErrorCodes::NoStepPossible:
				step_through_start_up_scrape_graphs();
				break;

			case ErrorCodes::Scrapeable_ErrorCodes::NoGraphBeingScraped:
				[[fallthrough]];
			case ErrorCodes::Scrapeable_ErrorCodes::UnknownScrapeError:
				[[fallthrough]];
			default:
				// No scrape is active (or waiting) but it's a cold start...this is weird so force a transition to normal operation.
				LogDebug(Channel::Devices, std::format("Emulated OneTouch device initialisation ({}) in an abnormal state -> forcing entry to normal operation", (OperatingStates::ColdStart == m_OpState) ? "COLD START" : "WARM START"));
				m_OpState = OperatingStates::NormalOperation;
				Status(Devices::DeviceStatus_Normal{});
				break;
			}
		}
		else
		{
			try
			{
				m_KeyCommand_ToSend = std::any_cast<KeyCommands>(scrape_step_outcome.value());
				LogDebug(Channel::Devices, std::format("Emulated OneTouch device: scrape in-progress - sending next command: {}", magic_enum::enum_name(m_KeyCommand_ToSend)));
			}
			catch (const std::bad_any_cast& eAC)
			{
				LogDebug(Channel::Devices, std::format("Failed trying to get key command for next step: error -> {}", eAC.what()));
			}
		}
	}

}
// namespace AqualinkAutomate::Devices
