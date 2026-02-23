#pragma once

#include <boost/units/base_units/metric/liter.hpp>
#include <boost/units/base_units/metric/minute.hpp>
#include <boost/units/base_units/us/gallon.hpp>
#include <boost/units/conversion.hpp>
#include <boost/units/operators.hpp>
#include <boost/units/physical_dimensions/volume.hpp>
#include <boost/units/physical_dimensions/time.hpp>
#include <boost/units/quantity.hpp>

namespace AqualinkAutomate::Units
{    

    using flow_rate_dimension = boost::mpl::divides<boost::units::volume_dimension, boost::units::time_dimension>::type;

    using gallons_per_minute_unit = boost::units::unit<flow_rate_dimension, boost::units::make_system<boost::units::us::gallon_base_unit, boost::units::metric::minute_base_unit>::type>;
    using liters_per_minute_unit = boost::units::unit<flow_rate_dimension, boost::units::make_system<boost::units::metric::liter_base_unit, boost::units::metric::minute_base_unit>::type>;

    BOOST_UNITS_STATIC_CONSTANT(gpm, gallons_per_minute_unit::unit_type);
    BOOST_UNITS_STATIC_CONSTANT(lpm, liters_per_minute_unit::unit_type);

    using gallons_per_minute = boost::units::quantity<gallons_per_minute_unit>;
    using liters_per_minute = boost::units::quantity<liters_per_minute_unit>;
    
}
// namespace AqualinkAutomate::Units
