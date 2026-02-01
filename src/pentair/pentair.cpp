#include "kernel/equipment_hub.h"
#include "logging/logging.h"
#include "options/options_pentair.h"
#include "pentair.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Pentair
{

	void Initialise(Kernel::HubLocator& hub_locator)
	{
	}

	void Configure(Kernel::HubLocator& hub_locator, const AqualinkAutomate::Options::Settings& settings)
	{
		auto pentair_settings_result = settings.Get<Pentair::Options::PentairSettings>();
		if (!pentair_settings_result)
		{
			///FIXME
			return;
		}

		const auto& pentair_settings = pentair_settings_result.value().get();

		auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
		if (nullptr == equipment_hub)
		{
			///FIXME
			return;
		}

		//---------------------------------------------------------------------
		// PENTAIR EQUIPMENT
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::PentairEquipment...");
	}

}
// namespace AqualinkAutomate::Pentair
