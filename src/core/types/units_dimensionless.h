#pragma once

#include <boost/units/static_constant.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/prefixes.hpp>

namespace AqualinkAutomate::Units
{

	typedef boost::units::make_scaled_unit<boost::units::si::dimensionless, boost::units::scale<10, boost::units::static_rational<-6>>>::type ppm_unit;

	BOOST_UNITS_STATIC_CONSTANT(ppm, ppm_unit);

	typedef boost::units::quantity<ppm_unit> ppm_quantity;

}
// namespace AqualinkAutomate::Units
