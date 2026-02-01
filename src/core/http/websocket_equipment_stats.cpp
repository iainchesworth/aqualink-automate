#include <cstdint>

#include "http/json/json_equipment.h"
#include "http/websocket_equipment_stats.h"
#include "http/websocket_event.h"
#include "http/websocket_event_types.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment_Stats::WebSocket_Equipment_Stats(Kernel::HubLocator& hub_locator) :
		Interfaces::IWebSocket<EQUIPMENTSTATS_WEBSOCKET_URL>(),
		m_StatsSlot()
	{
		m_StatisticsHub = hub_locator.Find<Kernel::StatisticsHub>();

		if (m_StatisticsHub)
		{
			m_StatsSlot = m_StatisticsHub->MessageCounts.Signal().connect(
				[this](uint64_t)
				{
					auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WS Stats Update", std::source_location::current());
					auto payload = HTTP::WebSocket_Event(HTTP::WebSocket_EventTypes::StatisticsUpdate, JSON::GenerateJson_Equipment_Stats(m_StatisticsHub)).Payload();
					zone->Value(payload.size());
					m_MessageQueue.push_back(std::move(payload));
				});
		}
	}

	std::optional<std::string> WebSocket_Equipment_Stats::DequeueMessage()
	{
		if (m_MessageQueue.empty())
		{
			return std::nullopt;
		}

		auto msg = std::move(m_MessageQueue.front());
		m_MessageQueue.pop_front();
		return msg;
	}

	void WebSocket_Equipment_Stats::OnOpen()
	{
	}

	void WebSocket_Equipment_Stats::OnMessage(const boost::beast::flat_buffer& buffer)
	{
	}

	void WebSocket_Equipment_Stats::OnPublish()
	{
	}

	void WebSocket_Equipment_Stats::OnClose()
	{
	}

	void WebSocket_Equipment_Stats::OnError()
	{
	}

}
// namespace AqualinkAutomate::HTTP
