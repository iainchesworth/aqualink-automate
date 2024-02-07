#pragma once

#include <source_location>
#include <string>

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

namespace AqualinkAutomate::Exceptions
{

	class GenericAqualinkException
	{
	public:
		GenericAqualinkException(std::string message, std::source_location location = std::source_location::current(), boost::stacktrace::stacktrace trace = boost::stacktrace::stacktrace());

	public:
		std::string& What();
		const std::string& What() const noexcept;
		const std::source_location& Where() const noexcept;
		const boost::stacktrace::stacktrace& StackTrace() const noexcept;

	private:
		std::string m_ExceptionMessage;
		const std::source_location m_SourceLocation;
		const boost::stacktrace::stacktrace m_StackTrace;
	};

}
// namespace AqualinkAutomate::Exceptions
