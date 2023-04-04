#pragma once

#include <functional>
#include <string>
#include <utility>

namespace AqualinkAutomate::Signals::CleanUp
{
	typedef std::function<void()> ActionFunc;
	typedef std::pair<std::string, ActionFunc> CleanUp;

	void Register(CleanUp cleanup);
	void PerformCleanUp();

}
// namespace AqualinkAutomate::Signals::CleanUp
