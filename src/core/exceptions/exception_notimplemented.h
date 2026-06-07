#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class NotImplemented : public GenericAqualinkException
	{
		static const std::string NOT_IMPLEMENTED_MESSAGE;

	public:
		NotImplemented();
	};

}
// namespace AqualinkAutomate::Exceptions
