#pragma once

#include <string>

#include <boost/system/error_code.hpp>
#include <magic_enum.hpp>

namespace AqualinkAutomate::ErrorCodes
{

	enum Factory_ErrorCodes
	{
		Error_UnknownFactoryError = 5000,
		Error_UnknownDeviceLabel = 5001,
		Error_CannotCastToJandyAuxillaryId = 5002,
		Error_FailedToCreateAuxillaryPtr = 5003,
		Error_ReceivedInvalidAuxillaryStatus = 5004
	};
	
	class Factory_ErrorCategory : public boost::system::error_category
	{
	public:
		static const Factory_ErrorCategory& Instance();

	public:
		virtual const char* name() const noexcept override;
		virtual std::string message(int ev) const override;
	};

}
// namespace AqualinkAutomate::ErrorCodes

namespace boost::system
{
	template<>
	struct is_error_code_enum<AqualinkAutomate::ErrorCodes::Factory_ErrorCodes> : public std::true_type {};
}

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::Factory_ErrorCodes e);
boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Factory_ErrorCodes e);

template <>
struct magic_enum::customize::enum_range<AqualinkAutomate::ErrorCodes::Factory_ErrorCodes>
{
	static constexpr int min = 5000;
	static constexpr int max = 5999;
	// (max - min) must be less than UINT16_MAX.
};
