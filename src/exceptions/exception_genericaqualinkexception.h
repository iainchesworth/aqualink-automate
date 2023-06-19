#pragma once

#include <stacktrace>
#include <stdexcept>
#include <string_view>

namespace AqualinkAutomate::Exceptions
{

	class GenericAqualinkException : public std::runtime_error
	{
	public:
		GenericAqualinkException(const std::string_view& message);

	public:
		const std::stacktrace& StackTrace() const;

	private:
		std::stacktrace m_StackTrace;
	};

}
// namespace AqualinkAutomate::Exceptions
