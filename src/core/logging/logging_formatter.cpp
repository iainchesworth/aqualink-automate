#include <cstdint>
#include <format>
#include <iomanip>
#include <string>

#include <boost/log/expressions.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <magic_enum/magic_enum.hpp>

#include "logging/logging_attributes.h"
#include "logging/logging_formatter.h"
#include "logging/logging_severity_levels.h"

namespace AqualinkAutomate::Logging
{

	void Formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
	{
		// Select the line layout from the record's OWN severity, not the channel's
		// configured filter level. The channel filter is a gate (does this record get
		// emitted at all); it must not decide whether THIS record renders the
		// file:line suffix. A Trace/Debug record always carries the source location.
		const auto record_severity_level = rec[severity].get<Severity>();

		auto debug_and_trace_formatter = [](auto& rec) -> std::string
		{
			return std::format(
				"{:08}: <{}>\t({}) [{}:{}] {}",
				rec[line_id].template get<uint32_t>(),
				magic_enum::enum_name(rec[severity].template get<Severity>()),
				magic_enum::enum_name(rec[channel].template get<Channel>()),
				rec[source_file].template get<std::string>(),
				rec[source_line].template get<uint32_t>(),
				rec[boost::log::expressions::smessage].template get<std::string>());
		};

		auto all_other_levels_formatter = [](auto& rec) -> std::string
		{
			return std::format(
				"{:08}: <{}>\t({}) {}",
				rec[line_id].template get<uint32_t>(),
				magic_enum::enum_name(rec[severity].template get<Severity>()),
				magic_enum::enum_name(rec[channel].template get<Channel>()),
				rec[boost::log::expressions::smessage].template get<std::string>());
		};

		switch (record_severity_level)
		{
		case Severity::Trace:
		case Severity::Debug:
			strm << debug_and_trace_formatter(rec);
			break;

		case Severity::Info:
		case Severity::Notify:
		case Severity::Warning:
		case Severity::Error:
		case Severity::Fatal:
		default:
			strm << all_other_levels_formatter(rec);
			break;
		}
	}

}
// namespace AqualinkAutomate::Logging
