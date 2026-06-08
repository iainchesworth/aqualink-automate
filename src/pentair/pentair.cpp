#include <memory>

#include "kernel/equipment_hub.h"
#include "logging/logging.h"
#include "equipment/pentair_equipment.h"
#include "options/options_pentair.h"
#include "pentair.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Pentair
{

	void Initialise(Kernel::HubLocator& hub_locator)
	{
		// Reserved early-initialisation hook, deliberately empty.  It mirrors
		// Jandy::Initialise() so both protocol subsystems present the same
		// two-phase Initialise()/Configure() API to the application bootstrap in
		// aqualink-automate.cpp: Initialise() runs before the hubs are fully
		// populated (nothing Pentair needs here yet), and all real wiring — locating
		// the EquipmentHub and registering PentairEquipment — happens in Configure().
		(void)hub_locator;
	}

	void Configure(Kernel::HubLocator& hub_locator, const AqualinkAutomate::Options::Settings& settings)
	{
		auto pentair_settings_result = settings.Get<Pentair::Options::PentairSettings>();
		if (!pentair_settings_result)
		{
			LogError(Channel::Main, "Failed to retrieve Pentair settings; Pentair equipment will not be configured");
			return;
		}

		const auto& pentair_settings = pentair_settings_result.value().get();

		auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
		if (nullptr == equipment_hub)
		{
			LogError(Channel::Main, "Failed to locate EquipmentHub; Pentair equipment will not be configured");
			return;
		}

		//---------------------------------------------------------------------
		// PENTAIR EQUIPMENT
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::PentairEquipment...");

		// PentairEquipment subscribes to decoded Pentair messages and discovers
		// devices (VSP pumps, chlorinator, controller) as their traffic appears,
		// mirroring how JandyEquipment discovers Jandy devices.
		equipment_hub->AddEquipment(std::make_unique<Equipment::PentairEquipment>(hub_locator));
	}

}
// namespace AqualinkAutomate::Pentair
