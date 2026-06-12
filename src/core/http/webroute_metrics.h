#pragma once

#include <memory>

#include "interfaces/iwebroute.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP
{
	// Prometheus convention: the metrics endpoint lives at the root, NOT under
	// /api.  It is always registered (read-only) and is subject to the same
	// SecurityConfig bearer enforcement as every other route (Prometheus scrapers
	// support bearer auth natively).
	inline constexpr char METRICS_ROUTE_URL[] = "/metrics";

	class WebRoute_Metrics : public Interfaces::IWebRoute<METRICS_ROUTE_URL>
	{
	public:
		explicit WebRoute_Metrics(Kernel::HubLocator& hub_locator);
		~WebRoute_Metrics() override = default;

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
