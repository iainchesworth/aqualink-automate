#include <map>
#include <stdexcept>

#include <magic_enum.hpp>

#include "logging/logging_attributes.h"
#include "logging/logging_severity_filter.h"

namespace AqualinkAutomate::Logging
{
	namespace SeverityFiltering
	{
		std::map<Channel, Severity> MinimumSeverityLevelPerChannel =
		{
			{Channel::Certificates, DEFAULT_SEVERITY},
			{Channel::Devices, DEFAULT_SEVERITY},
			{Channel::Equipment, DEFAULT_SEVERITY},
			{Channel::Exceptions, DEFAULT_SEVERITY},
			{Channel::Main, DEFAULT_SEVERITY},
			{Channel::Messages, DEFAULT_SEVERITY},
			{Channel::Options, DEFAULT_SEVERITY},
			{Channel::Platform, DEFAULT_SEVERITY},
			{Channel::Profiling, DEFAULT_SEVERITY},
			{Channel::Protocol, DEFAULT_SEVERITY},
			{Channel::Serial, DEFAULT_SEVERITY},
			{Channel::Signals, DEFAULT_SEVERITY},
			{Channel::Web, DEFAULT_SEVERITY}
		};

		void SetGlobalFilterLevel(Severity severity)
		{
			magic_enum::enum_for_each<Channel>([severity](auto const& channel) 
				{
					SetChannelFilterLevel(channel.value, severity); 
				}
			);
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
