#pragma once

#include <string>

#include <boost/system/error_code.hpp>

namespace AqualinkAutomate::ErrorCodes
{
	enum StringConversion_ErrorCodes
	{
		MalformedInput = 3000,
		UnknownStringConversionFailure
	};

	class StringConversion_ErrorCategory : public boost::system::error_category
	{
	public:
		static const StringConversion_ErrorCategory& Instance();

	public:
		virtual const char* name() const noexcept override;
		virtual std::string message(int ev) const override;
	};

}
// namespace AqualinkAutomate::ErrorCodes

namespace boost::system
{
	template<>
	struct is_error_code_enum<AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes> : public std::true_type {};
}

boost::system::error_code make_error_code(const AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes e);
boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes e);
