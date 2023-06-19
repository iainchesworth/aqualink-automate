#include <format>

#include "exceptions/exception_options_missingdependency.h"
#include "logging/logging.h"
#include "options/helpers/option_dependency_helper.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options
{

	void Helper_ValidateOptionDependencies(const boost::program_options::variables_map& vm, const AppOptionPtr& for_what, const AppOptionPtr& required_option)
	{
		const auto& for_what_long_name = (*((*for_what)())).long_name();
		const auto& required_option_long_name = (*((*required_option)())).long_name();

		if (vm.count(for_what_long_name) && !vm[for_what_long_name].defaulted())
		{
			if (vm.count(required_option_long_name) == 0 || vm[required_option_long_name].defaulted())
			{
				const auto error_message = std::format("Missing option dependency found: option {} requires option {}", for_what_long_name, required_option_long_name);
				LogDebug(Channel::Options, error_message);

				throw Exceptions::Options_MissingDependency(error_message);
			}
		}
	}

}
// namespace AqualinkAutomate::Options
