#pragma once

#include <string>

#include <boost/system/error_code.hpp>
#include <magic_enum.hpp>

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

template <>
struct magic_enum::customize::enum_range<AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes>
{
	static constexpr int min = 3000;
	static constexpr int max = 3999;
	// (max - min) must be less than UINT16_MAX.
};
