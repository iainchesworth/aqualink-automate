#pragma once

#include <stdexcept>
#include <string_view>

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

namespace AqualinkAutomate::Exceptions
{
	typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace> Traced;
	
	template <class E>
	void TracedThrow(const E& e) 
	{
		throw boost::enable_error_info(e) << Traced(boost::stacktrace::stacktrace());
	}

	class GenericAqualinkException : public std::runtime_error
	{
	public:
		GenericAqualinkException(const std::string_view& message);
	};

}
// namespace AqualinkAutomate::Exceptions
