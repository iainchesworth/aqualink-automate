#pragma once

#include <memory>

#include "interfaces/iwebpageroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char PAGE_INDEX_ROUTE_URL[] = "/";
	inline constexpr char PAGE_INDEX_TEMPLATE[] = "templates/index.html.mustache";

	class WebRoute_Page_Index : public Interfaces::IWebPageRoute<PAGE_INDEX_ROUTE_URL, PAGE_INDEX_TEMPLATE>
	{
	public:
		WebRoute_Page_Index(Kernel::HubLocator& hub_locator);

	public:
        virtual HTTP::Message OnRequest(HTTP::Request req) final;

	private:
		void PopulateMainActionButtons();
		void PopulateTriggerableButtons();

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
