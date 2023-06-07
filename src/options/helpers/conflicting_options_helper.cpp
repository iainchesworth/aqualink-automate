#include <format>

#include "exceptions/exception_options_conflictingoptions.h"
#include "logging/logging.h"
#include "options/helpers/conflicting_options_helper.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options
{

	void Helper_CheckForConflictingOptions(const boost::program_options::variables_map& vm, const AppOptionPtr& opt1, const AppOptionPtr& opt2)
	{
		const auto& opt1_long_name = (*((*opt1)())).long_name();
		const auto& opt2_long_name = (*((*opt2)())).long_name();

		if (vm.count(opt1_long_name) && !vm[opt1_long_name].defaulted() && vm.count(opt2_long_name) && !vm[opt2_long_name].defaulted())
		{
			const auto error_message = std::format("Conflicting options found on command line: option {} and option {}", opt1_long_name, opt2_long_name);
			LogDebug(Channel::Options, error_message);

			throw Exceptions::Options_ConflictingOptions(error_message);
		}
	}

}
// namespace AqualinkAutomate::Options
