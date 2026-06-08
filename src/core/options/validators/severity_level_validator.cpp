#include <string>
#include <vector>

#include <boost/any.hpp>

#include "logging/logging_channels.h"
#include "options/validators/severity_level_validator.h"
#include "options/validators/validate_enum_option.h"

namespace AqualinkAutomate::Logging
{

	void validate(boost::any& v, std::vector<std::string> const& values, Severity* /* target_type */, int)
	{
		// Delegate to the shared, empty-safe, case-insensitive enum validator.
		// (Severity enumerators are PascalCase, e.g. trace/TRACE -> Trace.)
		Options::Validators::ValidateEnumOption<Severity>(v, values, Channel::Main, "severity level");
	}

}
// namespace AqualinkAutomate::Logging
