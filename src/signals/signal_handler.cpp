#include <format>

#include "logging/logging.h"
#include "signals/signal_cleanup.h"
#include "signals/signal_handler.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Signals
{

	void SignalHandler(const int signal_number)
	{
		LogDebug(Channel::Signals, std::format("Handling signal: {}", signal_number));

		CleanUp::PerformCleanUp();
	}

}
// namespace AqualinkAutomate::Signals
