#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

    class HTTP_DuplicateRoute : public GenericAqualinkException
    {
        static constexpr std::string_view DUPLICATE_ROUTE_MESSAGE{""};

    public:
        HTTP_DuplicateRoute();
        HTTP_DuplicateRoute(const std::string &message);
    };

}
// namespace AqualinkAutomate::Exceptions
