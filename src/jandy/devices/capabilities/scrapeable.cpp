#include "jandy/devices/capabilities/scrapeable.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	void Scrapeable::ScrapingStart(ScrapeId scrape_graph_id, const uint32_t starting_index)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Scraping Capability -> ScrapingStart", std::source_location::current());

		if (auto graph_map_it = m_ScraperGraphs.find(scrape_graph_id); m_ScraperGraphs.end() == graph_map_it)
		{
			LogDebug(Channel::Devices, std::format("Scrape -> cannot start; graph id {} does not exist", scrape_graph_id));
		}
		else
		{
			LogDebug(Channel::Devices, std::format("Scrape -> starting active scrape of graph id {}", scrape_graph_id));
			m_ActiveScrape = { scrape_graph_id, ScraperIter::begin(m_ScraperGraphs.at(scrape_graph_id), starting_index) };
		}
	}

	std::expected<std::any, Scrapeable::ScrapingErrors> Scrapeable::ScrapingNext()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Scraping Capability -> ScrapingNext", std::source_location::current());

		if (!m_Stack_WaitingForPage.empty())
		{
			LogTrace(Channel::Devices, "Scrape -> is active; cannot step forward - waiting for screen page");
			return std::unexpected(ScrapingErrors::WaitingForPage);
		}
		if (!m_Stack_WaitingForMessage.empty())
		{
			LogTrace(Channel::Devices, "Scrape -> is active; cannot step forward - waiting for message");
			return std::unexpected(ScrapingErrors::WaitingForMessage);
		}
		else if (!m_ActiveScrape.has_value())
		{
			LogTrace(Channel::Devices, "Scrape -> is not active; cannot step forward");
			return std::unexpected(ScrapingErrors::NoGraphBeingScraped);
		}
		else
		{
			try
			{
				auto& active_scrape = m_ActiveScrape.value();
				auto& id = std::get<ScrapeId>(active_scrape);
				auto& it = std::get<ScraperIter>(active_scrape);

				if (ScraperIter::end(m_ScraperGraphs.at(id)) == it)
				{
					LogTrace(Channel::Devices, "Scrape -> is complete");
					m_ActiveScrape = std::nullopt;
					return std::unexpected(ScrapingErrors::NoStepPossible);
				}
				else
				{
					LogTrace(Channel::Devices, "Scrape -> is active; making next step");

					m_Stack_WaitingForMessage.push(Messages::JandyMessageIds::Status);
					m_Stack_WaitingForMessage.push(Messages::JandyMessageIds::Status);

					return std::get<Utility::ScreenDataPageGraphImpl::Edge>(*(++it)).key_command;
				}
			}
			catch (const std::bad_optional_access& eBOA)
			{
				LogTrace(Channel::Devices, std::format("Scrape -> was active but could not access graph (exception was -> {})", eBOA.what()));
				return std::unexpected(ScrapingErrors::NoGraphBeingScraped);
			}
		}
	}

}
// namespace AqualinkAutomate::Devices::Capabilities
