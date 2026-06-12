#pragma once

#include <memory>

#include "interfaces/icommanddispatcher.h"
#include "interfaces/iwebroute.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_IAQ_ROUTE_URL[] = "/api/equipment/iaq";

	// POST drives the AqualinkTouch (0x33) page UI from the emulated panel. Body field:
	//   { "select_button": N }  -> press the on-screen PageButton at index N (the index
	//                              carried in the master's IAQMessage_PageButton frames),
	//                              navigating the master's pages.
	// 400 on a bad value, 503 when no commandable IAQ/AqualinkTouch device is present.
	class WebRoute_Equipment_IAQ : public Interfaces::IWebRoute<EQUIPMENT_IAQ_ROUTE_URL>
	{
	public:
		explicit WebRoute_Equipment_IAQ(Kernel::HubLocator& hub_locator);

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Message HandlePost(const HTTP::Request& req);

	private:
		std::shared_ptr<Interfaces::ICommandDispatcher> m_CommandDispatcher{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
