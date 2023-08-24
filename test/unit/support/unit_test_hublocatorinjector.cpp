#include <memory>

#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/preferences_hub.h"
#include "kernel/statistics_hub.h"

#include "support/unit_test_hublocatorinjector.h"

namespace AqualinkAutomate::Test
{

	HubLocatorInjector::HubLocatorInjector()
	{
		Register(std::make_shared<Kernel::DataHub>());
		Register(std::make_shared<Kernel::EquipmentHub>());
		Register(std::make_shared<Kernel::PreferencesHub>());
		Register(std::make_shared<Kernel::StatisticsHub>());
	}

	HubLocatorInjector::~HubLocatorInjector()
	{
	}

}
// namespace AqualinkAutomate::Test
