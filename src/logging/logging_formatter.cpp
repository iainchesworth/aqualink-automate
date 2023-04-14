#include <cstdint>
#include <format>
#include <iomanip>
#include <string>

#include <boost/log/expressions.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <magic_enum.hpp>

#include "logging/logging_attributes.h"
#include "logging/logging_formatter.h"
#include "logging/logging_severity_filter.h"

namespace AqualinkAutomate::Logging
{

	void Formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
	{
		const auto current_severity_level = SeverityFiltering::GetChannelFilterLevel(rec[channel].get<Channel>());

		auto debug_and_trace_formatter = [](auto& rec, auto& strm) -> std::string 
		{
			return std::format(
				"{:08}: <{}>\t({}) [{}:{}] {}",
				rec[line_id].get<uint32_t>(),
				magic_enum::enum_name(rec[severity].get<Severity>()),
				magic_enum::enum_name(rec[channel].get<Channel>()),
				rec[source_file].get<std::string>(),
				rec[source_line].get<uint32_t>(),
				rec[boost::log::expressions::smessage].get<std::string>());
		};
		
		auto all_other_levels_formatter = [](auto& rec, auto& strm) -> std::string
		{
			return std::format(
				"{:08}: <{}>\t({}) {}",
				rec[line_id].get<uint32_t>(),
				magic_enum::enum_name(rec[severity].get<Severity>()),
				magic_enum::enum_name(rec[channel].get<Channel>()),
				rec[boost::log::expressions::smessage].get<std::string>());
		};

		switch (current_severity_level)
		{
		case Severity::Trace:
		case Severity::Debug:
			strm << debug_and_trace_formatter(rec, strm);
			break;

		case Severity::Info:
		case Severity::Notify:
		case Severity::Warning:
		case Severity::Error:
		case Severity::Fatal:
		default:
			strm << all_other_levels_formatter(rec, strm);
			break;
		}

		/*
		strm <<
			std::format(
				"{:08}: <{}>\t({})\t[{}:{}] {}",
				rec[line_id].get<uint32_t>(),
				boost::describe::enum_to_string(rec[severity].get<Severity>(), "UNKNOWN"),
				boost::describe::enum_to_string(rec[channel].get<Channel>(), "UNKNOWN"),
				rec[source_file].get<std::string>(),
				rec[source_line].get<uint32_t>(),
				rec[boost::log::expressions::smessage].get<std::string>());

		
			<< std::hex << std::setw(8) << std::setfill('0') << line_id
			<< std::dec << std::setfill(' ') << ": <" << severity << ">\t"
			<< "(" << channel << ") "
			<< " [" << source_file << ":" << source_line << "] "
			<< boost::log::expressions::if_(boost::log::expressions::has_attr(scope))
			[
				boost::log::expressions::stream << "(" << scope << ") "
			]
			<< boost::log::expressions::if_(boost::log::expressions::has_attr(tag_attr))
			[
				boost::log::expressions::stream << "[" << tag_attr << "] "
			]
			<< boost::log::expressions::if_(boost::log::expressions::has_attr(timeline))
			[
				boost::log::expressions::stream << "[" << timeline << "] "
			]
			<< boost::log::expressions::smessage;
		*/
	}

}
// namespace AqualinkAutomate::Logging
