#pragma once

#include <boost/log/utility/value_ref.hpp>

#include "logging/logging_attributes.h"
#include "logging/logging_channels.h"
#include "logging/logging_severity_levels.h"

namespace AqualinkAutomate::Logging
{
	namespace SeverityFiltering
	{
		const Channel DEFAULT_CHANNEL = Channel::Main;
		const Severity DEFAULT_SEVERITY = Severity::Info;

		void SetGlobalFilterLevel(Severity severity);
		void SetChannelFilterLevel(Channel channel, Severity severity);
		
		Severity GetChannelFilterLevel(Channel channel);

		bool PerChannelTest(boost::log::value_ref<Channel, tag::channel> const& channel, boost::log::value_ref<Severity, tag::severity> const& severity);
	}
	// namespace SeverityFiltering
}
// namespace AqualinkAutomate::Logging
