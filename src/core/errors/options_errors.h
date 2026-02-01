#pragma once

#include <string>

#include <boost/system/error_code.hpp>
#include <magic_enum/magic_enum.hpp>

namespace AqualinkAutomate::ErrorCodes
{
	enum Options_ErrorCodes
	{
		OptionsAreaAlreadyRegistered = 1,
		OptionsValidationFailed,
		OptionsParsingFailed,
		OptionsHandlingFailed,
		UnknownOptionsError
	};

	class Options_ErrorCategory : public boost::system::error_category
	{
	public:
		static const Options_ErrorCategory& Instance();

	public:
		virtual const char* name() const noexcept override;
		virtual std::string message(int ev) const override;
	};

}
// namespace AqualinkAutomate::ErrorCodes

namespace boost::system
{
	template<>
	struct is_error_code_enum<AqualinkAutomate::ErrorCodes::Options_ErrorCodes> : public std::true_type
	{
	};
}

boost::system::error_code make_error_code(const AqualinkAutomate::ErrorCodes::Options_ErrorCodes e);
boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Options_ErrorCodes e);

template <>
struct magic_enum::customize::enum_range<AqualinkAutomate::ErrorCodes::Options_ErrorCodes>
{
	static constexpr int min = 101;
	static constexpr int max = 199;
	// (max - min) must be less than UINT16_MAX.
};
