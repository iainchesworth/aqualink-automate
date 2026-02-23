#pragma once

#include <chrono>

#include "developer/async_operation_tracker.h"
#include "developer/coroutine_lifetime_tracker.h"
#include "developer/signal_connection_tracker.h"

namespace AqualinkAutomate::Developer
{

	void DumpAllDiagnostics(std::chrono::steady_clock::duration threshold = std::chrono::seconds(5));

}
// namespace AqualinkAutomate::Developer
