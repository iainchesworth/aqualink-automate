#include <map>
#include <stdexcept>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_utility.hpp>

#include "logging/logging_attributes.h"
#include "logging/logging_severity_filter.h"

namespace AqualinkAutomate::Logging
{
	namespace SeverityFiltering
	{
		std::map<Channel, Severity> MinimumSeverityLevelPerChannel =
		{
			{Channel::Certificates, DEFAULT_SEVERITY},
			{Channel::Coroutines, DEFAULT_SEVERITY},
			{Channel::Developer, DEFAULT_SEVERITY},
			{Channel::Devices, DEFAULT_SEVERITY},
			{Channel::Equipment, DEFAULT_SEVERITY},
			{Channel::Exceptions, DEFAULT_SEVERITY},
			{Channel::Main, DEFAULT_SEVERITY},
			{Channel::Messages, DEFAULT_SEVERITY},
			{Channel::Mqtt, DEFAULT_SEVERITY},
			{Channel::Navigation, DEFAULT_SEVERITY},
			{Channel::Options, DEFAULT_SEVERITY},
			{Channel::Platform, DEFAULT_SEVERITY},
			{Channel::Profiling, DEFAULT_SEVERITY},
			{Channel::Protocol, DEFAULT_SEVERITY},
			{Channel::Scraping, DEFAULT_SEVERITY},
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
			if (const auto it = MinimumSeverityLevelPerChannel.find(channel); it != MinimumSeverityLevelPerChannel.end())
			{
				return it->second;
			}

			return DEFAULT_SEVERITY;
		}

		bool ShouldLog(Channel channel, Severity severity)
		{
			return severity >= GetChannelFilterLevel(channel);
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
