#pragma once

#include <filesystem>
#include <utility>

#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/assert/source_location.hpp>

#include "logging/global_logger.h"
#include "logging/logging_attributes.h"

namespace AqualinkAutomate::Logging
{

	template<typename MESSAGE>
	constexpr auto Log(MESSAGE log_message, Channel channel, Severity severity, const boost::source_location location)
	{
		auto GetGlobalLogger = [](auto channel) -> Logger&
		{
			switch (channel)
			{
			case Channel::Certificates:
				return GlobalLogger_Certificates::get();

			case Channel::Devices:
				return GlobalLogger_Devices::get();

			case Channel::Equipment:
				return GlobalLogger_Equipment::get();

			case Channel::Exceptions:
				return GlobalLogger_Exceptions::get();

			case Channel::Main:
				return GlobalLogger_Main::get();

			case Channel::Messages: 
				return GlobalLogger_Messages::get();

			case Channel::Options:
				return GlobalLogger_Options::get();

			case Channel::Platform:
				return GlobalLogger_Platform::get();

			case Channel::Profiling:
				return GlobalLogger_Profiling::get();

			case Channel::Protocol:
				return GlobalLogger_Protocol::get();

			case Channel::Serial: 
				return GlobalLogger_Serial::get();

			case Channel::Signals:
				return GlobalLogger_Signals::get();

			case Channel::Web:
				return GlobalLogger_Web::get();

			default:
				// This is a problem...there's a channel type that has not been 
				// added to the above list...default to the Main channel.
				return GlobalLogger_Main::get();
			}
		};

		Logger& lg = GetGlobalLogger(channel);
		BOOST_LOG_SEV(lg, severity) 
			<< boost::log::add_value(source_file, std::filesystem::path(location.file_name()).filename().string())
			<< boost::log::add_value(source_line, location.line())
			<< log_message;
	}

}
// namespace AqualinkAutomate::Logging

template<typename MESSAGE >
constexpr void LogTrace(AqualinkAutomate::Logging::Channel channel, MESSAGE log_message, const boost::source_location location = BOOST_CURRENT_LOCATION)
{
	AqualinkAutomate::Logging::Log(log_message, channel, AqualinkAutomate::Logging::Severity::Trace, location);
}

template<typename MESSAGE>
constexpr void LogDebug(AqualinkAutomate::Logging::Channel channel, MESSAGE log_message, const boost::source_location location = BOOST_CURRENT_LOCATION)
{
	AqualinkAutomate::Logging::Log(log_message, channel, AqualinkAutomate::Logging::Severity::Debug, location);
}

template<typename MESSAGE>
constexpr void LogInfo(AqualinkAutomate::Logging::Channel channel, MESSAGE log_message, const boost::source_location location = BOOST_CURRENT_LOCATION)
{
	AqualinkAutomate::Logging::Log(log_message, channel, AqualinkAutomate::Logging::Severity::Info, location);
}

template<typename MESSAGE>
constexpr void LogNotify(AqualinkAutomate::Logging::Channel channel, MESSAGE log_message, const boost::source_location location = BOOST_CURRENT_LOCATION)
{
	AqualinkAutomate::Logging::Log(log_message, channel, AqualinkAutomate::Logging::Severity::Notify, location);
}

template<typename MESSAGE>
constexpr void LogWarning(AqualinkAutomate::Logging::Channel channel, MESSAGE log_message, const boost::source_location location = BOOST_CURRENT_LOCATION)
{
	AqualinkAutomate::Logging::Log(log_message, channel, AqualinkAutomate::Logging::Severity::Warning, location);
}

template<typename MESSAGE>
constexpr void LogError(AqualinkAutomate::Logging::Channel channel, MESSAGE log_message, const boost::source_location location = BOOST_CURRENT_LOCATION)
{
	AqualinkAutomate::Logging::Log(log_message, channel, AqualinkAutomate::Logging::Severity::Error, location);
}

template<typename MESSAGE>
constexpr void LogFatal(AqualinkAutomate::Logging::Channel channel, MESSAGE log_message, const boost::source_location location = BOOST_CURRENT_LOCATION)
{
	AqualinkAutomate::Logging::Log(log_message, channel, AqualinkAutomate::Logging::Severity::Fatal, location);
}
