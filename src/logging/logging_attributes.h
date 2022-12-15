#pragma once

#include <cstdint>
#include <string>

#include <boost/log/attributes.hpp>
#include <boost/log/expressions/keyword.hpp>

#include "logging/logging_channels.h"
#include "logging/logging_severity_levels.h"

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", uint32_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", AqualinkAutomate::Logging::Severity)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", AqualinkAutomate::Logging::Channel)
BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr, "Tag", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(scope, "Scope", boost::log::attributes::named_scope::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(timeline, "Timeline", boost::log::attributes::timer::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_line, "Line", uint32_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_file, "File", std::string)

namespace AqualinkAutomate::Logging
{

	//

}
// namespace AqualinkAutomate::Logging
