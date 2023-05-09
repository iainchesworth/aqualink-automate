#pragma once

#include <string>

#include <crow/logging.h>

namespace AqualinkAutomate::HTTP
{

	class CrowCustomLogger : public crow::ILogHandler
	{
	public:
		CrowCustomLogger();

	public:
		void log(std::string message, crow::LogLevel level);
	};

}
// namespace AqualinkAutomate::HTTP
