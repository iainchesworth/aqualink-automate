#pragma once

#include <source_location>
#include <stacktrace>
#include <string>

namespace AqualinkAutomate::Exceptions
{

	class GenericAqualinkException
	{
	public:
		GenericAqualinkException(std::string message, std::source_location location = std::source_location::current(), std::stacktrace trace = std::stacktrace::current());

	public:
		std::string& What();
		const std::string& What() const noexcept;
		const std::source_location& Where() const noexcept;
		const std::stacktrace& StackTrace() const noexcept;

	private:
		std::string m_ExceptionMessage;
		const std::source_location m_SourceLocation;
		const std::stacktrace m_StackTrace;
	};

}
// namespace AqualinkAutomate::Exceptions
