#pragma once

#include <string>

#include <boost/system/error_code.hpp>
#include <magic_enum.hpp>

namespace AqualinkAutomate::ErrorCodes
{

	enum Message_ErrorCodes
	{
		Error_InvalidMessageData = 1000,
		Error_CannotFindGenerator,
		Error_UnknownMessageType,
		Error_GeneratorFailed,
		Error_FailedToSerialize,
		Error_FailedToDeserialize
	};

	class Message_ErrorCategory : public boost::system::error_category
	{
	public:
		static const Message_ErrorCategory& Instance();

	public:
		virtual const char* name() const noexcept override;
		virtual std::string message(int ev) const override;
	};

}
// namespace AqualinkAutomate::ErrorCodes

namespace boost::system
{
	template<>
	struct is_error_code_enum<AqualinkAutomate::ErrorCodes::Message_ErrorCodes> : public std::true_type {};
}

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::Message_ErrorCodes e);
boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Message_ErrorCodes e);

template <>
struct magic_enum::customize::enum_range<AqualinkAutomate::ErrorCodes::Message_ErrorCodes>
{
	static constexpr int min = 1000;
	static constexpr int max = 1999;
	// (max - min) must be less than UINT16_MAX.
};
