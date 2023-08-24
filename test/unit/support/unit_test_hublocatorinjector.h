#pragma once

#include "kernel/hub_locator.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Test
{

	class HubLocatorInjector : public Kernel::HubLocator
	{
	public:
		HubLocatorInjector();
		virtual ~HubLocatorInjector();
	};

}
// namespace AqualinkAutomate::Test
