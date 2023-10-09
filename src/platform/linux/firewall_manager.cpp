#include "developer/firewall_manager.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer::FirewallUtils
{

	void CheckAndConfigureExceptions()
	{
		LogTrace(Channel::Platform, "No firewall management actions taken");
	}

}
// namespace AqualinkAutomate::Developer::FirewallUtils
