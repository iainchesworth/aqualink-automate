#pragma once

#include <boost/log/core/record_view.hpp>
#include <boost/log/utility/formatting_ostream.hpp>

namespace AqualinkAutomate::Logging
{

	void Formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm);

}
// namespace AqualinkAutomate::Logging
