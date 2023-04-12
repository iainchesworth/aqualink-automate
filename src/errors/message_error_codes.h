#pragma once

#include <boost/system/error_code.hpp>

namespace AqualinkAutomate::ErrorCodes
{
	enum Message_ErrorCodes
	{
		Error_InvalidMessageData = 1000,
		Error_UnknownMessageType
	};

	class Message_ErrorCategory : public boost::system::error_category
	{
	public:
		const char* name() const noexcept override;
		std::string message(int ev) const override;
	};

}
// namespace AqualinkAutomate::ErrorCodes

namespace boost::system
{
	template<>
	struct is_error_code_enum<AqualinkAutomate::ErrorCodes::Message_ErrorCodes> : public std::true_type {};
}

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::Message_ErrorCodes e);
