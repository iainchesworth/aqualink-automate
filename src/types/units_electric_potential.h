#pragma once

#include <boost/units/static_constant.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/prefixes.hpp>

namespace AqualinkAutomate::Units
{

    typedef boost::units::make_scaled_unit<boost::units::si::electric_potential, boost::units::scale<10, boost::units::static_rational< -3>>>::type millivolt_unit;

    BOOST_UNITS_STATIC_CONSTANT(millivolt, millivolt_unit);
    BOOST_UNITS_STATIC_CONSTANT(millivolts, millivolt_unit);

    typedef boost::units::quantity<millivolt_unit> millivolt_quantity;

}
// namespace AqualinkAutomate::Units
