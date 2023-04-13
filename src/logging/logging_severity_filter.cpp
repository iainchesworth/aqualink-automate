#include <map>
#include <stdexcept>

#include "logging/logging_attributes.h"
#include "logging/logging_severity_filter.h"
#include "utility/describe_enumerators_as_array.h"

namespace AqualinkAutomate::Logging
{
	namespace SeverityFiltering
	{
		std::map<Channel, Severity> MinimumSeverityLevelPerChannel =
		{
			{Channel::Exceptions, DEFAULT_SEVERITY},
			{Channel::Main, DEFAULT_SEVERITY},
			{Channel::Messages, DEFAULT_SEVERITY},
			{Channel::Options, DEFAULT_SEVERITY},
			{Channel::Platform, DEFAULT_SEVERITY},
			{Channel::Serial, DEFAULT_SEVERITY},
			{Channel::Signals, DEFAULT_SEVERITY}
		};

		void SetGlobalFilterLevel(Severity severity)
		{
			for (auto const& channel : AqualinkAutomate::Utility::describe_enumerators_as_array<Channel>())
			{
				SetChannelFilterLevel(channel.value, severity);
			}
		}

		void SetChannelFilterLevel(Channel channel, Severity severity)
		{
			MinimumSeverityLevelPerChannel[channel] = severity;
		}

		Severity GetChannelFilterLevel(Channel channel)
		{
			return MinimumSeverityLevelPerChannel[channel];
		}

		bool PerChannelTest(boost::log::value_ref<Channel, tag::channel> const& channel, boost::log::value_ref<Severity, tag::severity> const& severity)
		{
			try
			{
				return (severity >= MinimumSeverityLevelPerChannel.at(*channel));
			}
			catch (const std::out_of_range&)
			{
				return true;
			}
		}
	}
	// namespace SeverityFiltering
}
// namespace AqualinkAutomate::Logging
