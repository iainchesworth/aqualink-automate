#include "logging/global_logger.h"

namespace AqualinkAutomate::Logging
{
	BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(GlobalLogger_Exceptions, Logger, (boost::log::keywords::channel = Channel::Exceptions));
	BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(GlobalLogger_Main, Logger, (boost::log::keywords::channel = Channel::Main));
	BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(GlobalLogger_Options, Logger, (boost::log::keywords::channel = Channel::Options));
	BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(GlobalLogger_Platform, Logger, (boost::log::keywords::channel = Channel::Platform));
	BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(GlobalLogger_Serial, Logger, (boost::log::keywords::channel = Channel::Serial));
	BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(GlobalLogger_Signals, Logger, (boost::log::keywords::channel = Channel::Signals));

}
// namespace AqualinkAutomate::Logging
