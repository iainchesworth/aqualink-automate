#pragma once

#include <magic_enum/magic_enum.hpp>

namespace AqualinkAutomate::ErrorCodes
{
	// Options_ErrorCodes is used purely as the error type of the option-pipeline
	// std::expected results (std::expected<Settings, Options_ErrorCodes>).  It is
	// never converted into a boost::system::error_code, so it deliberately has no
	// error_category / make_error_code machinery (that boilerplate was previously
	// declared but never defined, i.e. dead, unlinkable code).
	enum class Options_ErrorCodes : int
	{
		OptionsAreaAlreadyRegistered = 101,
		OptionsValidationFailed,
		OptionsParsingFailed,
		OptionsHandlingFailed,
		UnknownOptionsError
	};

}
// namespace AqualinkAutomate::ErrorCodes

template <>
struct magic_enum::customize::enum_range<AqualinkAutomate::ErrorCodes::Options_ErrorCodes>
{
	static constexpr int min = 101;
	static constexpr int max = 105;
	// (max - min) must be less than UINT16_MAX.
};
