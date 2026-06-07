#pragma once

#include <boost/log/utility/value_ref.hpp>

#include "logging/logging_attributes.h"
#include "logging/logging_channels.h"
#include "logging/logging_severity_levels.h"

namespace AqualinkAutomate::Logging
{
	namespace SeverityFiltering
	{
		inline constexpr Channel DEFAULT_CHANNEL = Channel::Main;
		inline constexpr Severity DEFAULT_SEVERITY = Severity::Info;

		void SetGlobalFilterLevel(Severity severity);
		void SetChannelFilterLevel(Channel channel, Severity severity);
		
		[[nodiscard]] Severity GetChannelFilterLevel(Channel channel);
		[[nodiscard]] bool ShouldLog(Channel channel, Severity severity);

		[[nodiscard]] bool PerChannelTest(boost::log::value_ref<Channel, tag::channel> const& channel, boost::log::value_ref<Severity, tag::severity> const& severity);
	}
	// namespace SeverityFiltering
}
// namespace AqualinkAutomate::Logging
