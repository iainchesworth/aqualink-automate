#pragma once

#include <memory>

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::History
{
	class HistoryService;
}

namespace AqualinkAutomate::HTTP
{
	inline constexpr char HISTORY_SERIES_ROUTE_URL[] = "/api/history/series";

	// History read API (WS2). One route, two behaviours:
	//   GET /api/history/series                       -> list series metadata
	//   GET /api/history/series?key=<k>&from&to&max_points -> bucket-averaged points
	// Series keys contain '/', so the key travels as a query parameter rather
	// than a path segment. Returns 503 when history is disabled (service null),
	// 404 for an unknown key, 400 for a bad range.
	class WebRoute_History : public Interfaces::IWebRoute<HISTORY_SERIES_ROUTE_URL>
	{
	public:
		explicit WebRoute_History(std::shared_ptr<History::HistoryService> service);
		~WebRoute_History() override = default;

	public:
		HTTP::Response OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<History::HistoryService> m_Service;
	};

}
// namespace AqualinkAutomate::HTTP
