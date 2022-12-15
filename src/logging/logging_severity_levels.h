#pragma once

#include <boost/describe.hpp>

namespace AqualinkAutomate::Logging
{
	BOOST_DEFINE_ENUM_CLASS(Severity, Trace, Debug, Info, Notify, Warning, Error, Fatal);
}
// namespace AqualinkAutomate::Logging