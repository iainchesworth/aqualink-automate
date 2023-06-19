#include <iostream>

#include <boost/core/null_deleter.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/phoenix/bind/bind_function.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

#include "logging/logging_attributes.h"
#include "logging/logging_channels.h"
#include "logging/logging_formatter.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"
#include "logging/logging_severity_levels.h"

namespace AqualinkAutomate::Logging
{

void Initialise()
{
	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_sink;
	boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
	boost::log::core::get()->add_sink(sink);

	sink->set_formatter(&Formatter);

	auto severity_filter = boost::phoenix::bind(&SeverityFiltering::PerChannelTest, channel.or_default(SeverityFiltering::DEFAULT_CHANNEL), severity.or_default(SeverityFiltering::DEFAULT_SEVERITY));
	sink->set_filter(severity_filter);

	boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
	sink->locked_backend()->add_stream(stream);

	boost::log::add_common_attributes();
	boost::log::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());;
}

}
// namespace AqualinkAutomate::Logging
