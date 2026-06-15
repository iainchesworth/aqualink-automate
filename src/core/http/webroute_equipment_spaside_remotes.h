#pragma once

#include <memory>

#include "interfaces/ispasideremotecontroller.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/preferences_hub.h"

namespace AqualinkAutomate::Preferences { class PreferencesService; }

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_SPASIDE_REMOTES_ROUTE_URL[] = "/api/equipment/spaside-remotes";

	// GET  -> snapshot of every spa-side remote (address, class, emulated, LEDs, last press), plus
	//         the controller's decoded button->function assignments and any user-requested ones.
	// POST -> {"action":"press","address":<n>,"button":<n>} injects a momentary press on the
	//         emulated remote at that address (the master then actuates its configured function); or
	//         {"action":"assign","switch":<n>,"button":<n>,"function":<s>} programs an assignment
	//         over the bus (and persists the request to PreferencesHub when a controller accepts it).
	class WebRoute_Equipment_SpasideRemotes : public Interfaces::IWebRoute<EQUIPMENT_SPASIDE_REMOTES_ROUTE_URL>
	{
	public:
		// preferences_service may be null (e.g. dev-mode/tests) -> assign requests still program the
		// controller, they just are not persisted.
		WebRoute_Equipment_SpasideRemotes(Kernel::HubLocator& hub_locator, std::shared_ptr<Preferences::PreferencesService> preferences_service = nullptr);
		~WebRoute_Equipment_SpasideRemotes() override = default;

	public:
		HTTP::Response OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Response HandleGet(const HTTP::Request& req);
		HTTP::Response HandlePost(const HTTP::Request& req);

	private:
		// Non-owning: the controller is owned by the application for its whole lifetime. nullptr
		// when no controller is registered (e.g. dev-mode/replay), in which case GET returns an
		// empty list and POST is rejected with 503.
		std::shared_ptr<Interfaces::ISpasideRemoteController> m_Controller;

		// The decoded spa-switch button->function assignments (from the iAQ/OneTouch config UI) live
		// on the DataHub; GET surfaces them so the UI can label buttons with their real function.
		std::shared_ptr<Kernel::DataHub> m_DataHub;

		// User-requested assignments are persisted here (desired state); GET surfaces them as
		// `requested` and an accepted assign records the request. Null when no persistence is wired.
		std::shared_ptr<Kernel::PreferencesHub> m_PreferencesHub;
		std::shared_ptr<Preferences::PreferencesService> m_PreferencesService;
	};
}
// namespace AqualinkAutomate::HTTP
