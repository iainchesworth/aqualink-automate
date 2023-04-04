#include <format>
#include <vector>

#include "logging/logging.h"
#include "signals/signal_cleanup.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Signals::CleanUp
{
	std::vector<CleanUp> CleanUpActionCollection;

	void PerformCleanUp()
	{
		LogDebug(Channel::Signals, "Commencing clean-up actions");

		for (auto& [desc, cleanup_action] : CleanUpActionCollection)
		{
			try
			{
				LogDebug(Channel::Signals, std::format("Actioning clean-up for {}", desc));
				cleanup_action();
			}
			catch (const std::exception& ex)
			{
				LogError(Channel::Signals, std::format("Unknown exception while actioning clean-up for {}: {}", desc, ex.what()));
			}
		}
	}

	void Register(CleanUp cleanup)
	{
		CleanUpActionCollection.push_back(cleanup);
	}

}
// namespace AqualinkAutomate::Signals::CleanUp
