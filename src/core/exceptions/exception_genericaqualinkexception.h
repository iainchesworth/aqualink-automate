#pragma once

#include <exception>
#include <source_location>
#include <string>

#include <boost/stacktrace.hpp>

namespace AqualinkAutomate::Exceptions
{

	class GenericAqualinkException : public std::exception
	{
	public:
		GenericAqualinkException(std::string message, std::source_location location = std::source_location::current(), boost::stacktrace::stacktrace trace = boost::stacktrace::stacktrace());

	public:
		const char* what() const noexcept override;
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
