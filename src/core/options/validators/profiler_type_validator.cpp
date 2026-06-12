#include <string>
#include <vector>

#include <boost/any.hpp>

#include "logging/logging_channels.h"
#include "options/validators/profiler_type_validator.h"
#include "options/validators/validate_enum_option.h"

namespace AqualinkAutomate::Types
{

	void validate(boost::any& v, std::vector<std::string> const& values, ProfilerTypes* /* target_type */, int)
	{
		// Delegate to the shared, empty-safe, case-insensitive enum validator.
		// (ProfilerTypes enumerators are PascalCase, e.g. tracy/TRACY -> Tracy.)
		Options::Validators::ValidateEnumOption<ProfilerTypes>(v, values, AqualinkAutomate::Logging::Channel::Main, "profiler");
	}

}
// namespace AqualinkAutomate::Types
