#pragma once

#include <memory>

#include "interfaces/ispasideremotecontroller.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_SPASIDE_REMOTES_ROUTE_URL[] = "/api/equipment/spaside-remotes";

	// GET  -> snapshot of every spa-side remote (address, class, emulated, LEDs, last press).
	// POST -> {"action":"press","address":<n>,"button":<n>} injects a momentary press on the
	//         emulated remote at that address (the master then actuates its configured function).
	class WebRoute_Equipment_SpasideRemotes : public Interfaces::IWebRoute<EQUIPMENT_SPASIDE_REMOTES_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_SpasideRemotes(Kernel::HubLocator& hub_locator);
		~WebRoute_Equipment_SpasideRemotes() override = default;

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Message HandleGet(const HTTP::Request& req);
		HTTP::Message HandlePost(const HTTP::Request& req);

	private:
		// Non-owning: the controller is owned by the application for its whole lifetime. nullptr
		// when no controller is registered (e.g. dev-mode/replay), in which case GET returns an
		// empty list and POST is rejected with 503.
		std::shared_ptr<Interfaces::ISpasideRemoteController> m_Controller;

		// The decoded spa-switch button->function assignments (from the iAQ/OneTouch config UI) live
		// on the DataHub; GET surfaces them so the UI can label buttons with their real function.
		std::shared_ptr<Kernel::DataHub> m_DataHub;
	};
}
// namespace AqualinkAutomate::HTTP
