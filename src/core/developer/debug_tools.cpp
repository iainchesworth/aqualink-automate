#include "developer/debug_tools.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

	void DumpAllDiagnostics(std::chrono::steady_clock::duration threshold)
	{
		LogInfo(Channel::Developer, "===== BEGIN DIAGNOSTICS DUMP =====");

		CoroutineLifetimeTracker::Instance().DumpActive();
		CoroutineLifetimeTracker::Instance().DumpSuspendedLongerThan(threshold);

		SignalConnectionTracker::Instance().DumpActive();
		SignalConnectionTracker::Instance().DumpOlderThan(threshold);

		AsyncOperationTracker::Instance().DumpPending();
		AsyncOperationTracker::Instance().DumpStuck(threshold);

		LogInfo(Channel::Developer, "===== END DIAGNOSTICS DUMP =====");
	}

}
// namespace AqualinkAutomate::Developer
