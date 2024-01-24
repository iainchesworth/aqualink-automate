#include <format>

#include "developer/instance_counter.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{
	const InstanceCounter& GetInstanceCounter()
	{
		static LoggingReporter logging_reporter;
		return logging_reporter;
	}

	void LoggingReporter::ReportConstruction(std::string_view object_type_name, std::size_t total_instances) const noexcept
	{
		LogTrace(Channel::Main, std::format("Construction of type: {} -> total instances: {}", object_type_name, total_instances));
	}

	void LoggingReporter::ReportDestruction(std::string_view object_type_name, std::size_t total_instances) const noexcept
	{
		LogTrace(Channel::Main, std::format("Destruction of type: {} -> remaining instances: {}", object_type_name, total_instances));
	}

}
// namespace AqualinkAutomate::Developer
