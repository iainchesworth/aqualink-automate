#include "http/crow_custom_logger.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	CrowCustomLogger::CrowCustomLogger() : 
		crow::ILogHandler()
	{
	}

	void CrowCustomLogger::log(std::string message, crow::LogLevel level)
	{
		switch (level)
		{
		case crow::LogLevel::Debug:
			LogDebug(Channel::Web, message);
			break;

		case crow::LogLevel::Info:
			LogInfo(Channel::Web, message);
			break;

		case crow::LogLevel::Warning:
			LogWarning(Channel::Web, message);
			break;

		case crow::LogLevel::Error:
			LogError(Channel::Web, message);
			break;

		case crow::LogLevel::Critical:
			LogFatal(Channel::Web, message);
			break;

		default:
			LogNotify(Channel::Web, message);
			break;
		}
	}

}
// namespace AqualinkAutomate::HTTP
