#pragma once

#include <any>
#include <cstdint>
#include <format>
#include <optional>
#include <stack>
#include <tuple>
#include <unordered_map>

#include <magic_enum.hpp>
#include <tl/expected.hpp>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/errors/jandy_errors_scrapeable.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/utility/screen_data_page_graph.h"
#include "jandy/utility/screen_data_page_processor.h"
#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_traverse.h"
#include "jandy/utility/slot_connection_manager.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices::Capabilities
{

	class Scrapeable
	{
	public:
		using ScrapeId = uint32_t;
		using ScraperGraph = Utility::ScreenDataPageGraph;
		using ScraperIter = Utility::ScreenDataPageGraphImpl::ForwardIterator;

	public:
		using GraphDataMap = std::unordered_map<ScrapeId, ScraperGraph>;
		using GraphIterMap = std::unordered_map<ScrapeId, ScraperIter>;

	public:
		template<typename... MESSAGE_TYPES>
		Scrapeable(const Devices::JandyDeviceType device_id, GraphDataMap graphs, MESSAGE_TYPES ...) :
			m_ScraperGraphs(graphs),
			m_ActiveScrape(std::nullopt),
			m_Stack_WaitingFor_SlotManager(),
			m_Stack_WaitingForPage(),
			m_Stack_WaitingForMessage(),
			m_ParentDeviceId(device_id)
		{
			auto slot_waitingformessage = [&](const auto& msg) -> void
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Scraping Capability -> Waiting For Message", BOOST_CURRENT_LOCATION);

				if (m_ParentDeviceId != msg.Destination().Id())
				{
					LogDebug(Channel::Devices, std::format("Scrape -> received message for incorrect device: expected -> {}, receiveed -> {}", m_ParentDeviceId(), msg.Destination().Id()));
				}
				else if (!m_ActiveScrape.has_value())
				{
					LogTrace(Channel::Devices, "Scrape -> nothing active; ignoring messages");
				}
				else if (m_Stack_WaitingForMessage.empty())
				{
					LogTrace(Channel::Devices, "Scrape -> is active; no messages being waited upon");
				}
				else if (m_Stack_WaitingForMessage.top() != msg.Id())
				{
					LogTrace(
						Channel::Devices, 
						std::format(
							"Scrape -> is active; message type {} (device: {}) does not match awaited message type {} (device: {})",
							magic_enum::enum_name(m_Stack_WaitingForMessage.top()), 
							m_ParentDeviceId(),
							magic_enum::enum_name(msg.Id()),
							msg.Destination().Id()
						));
				}
				else
				{
					LogTrace(Channel::Devices, std::format("Scrape -> is active; received awaited message type {}", magic_enum::enum_name(msg.Id())));
					m_Stack_WaitingForMessage.pop();
				}
			};

			// Unpack the parameter pack and register each associated message with the scraper's lambda.
			(m_Stack_WaitingFor_SlotManager.RegisterSlot_FilterByDeviceId<MESSAGE_TYPES>(slot_waitingformessage, device_id()), ...);
		}

	public:
		void ScrapingStart(ScrapeId scrape_graph_id, const uint32_t starting_index = 1);

	public:
		tl::expected<std::any, ErrorCodes::Scrapeable_ErrorCodes> ScrapingNext();

	private:
		GraphDataMap m_ScraperGraphs;
		std::optional<std::tuple<ScrapeId, ScraperIter>> m_ActiveScrape;

	private:
		Utility::SlotConnectionManager m_Stack_WaitingFor_SlotManager;
		std::stack<Utility::ScreenDataPageTypes> m_Stack_WaitingForPage;
		std::stack<Messages::JandyMessageIds> m_Stack_WaitingForMessage;
		const Devices::JandyDeviceType m_ParentDeviceId;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
