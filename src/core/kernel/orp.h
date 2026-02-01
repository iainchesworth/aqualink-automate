#pragma once

#include <boost/cstdfloat.hpp>

#include "types/units_electric_potential.h"

using namespace AqualinkAutomate::Units;

namespace AqualinkAutomate::Kernel
{

    class ORP 
    {
    public:
        ORP(boost::float64_t value_in_mV);

    public:
        Units::millivolt_quantity operator()() const;

    public:
        ORP& operator=(Units::millivolt_quantity value_in_mV);
        ORP& operator=(boost::float64_t value_in_mV);

    public:
        bool operator==(const ORP& other) const;
        bool operator==(const boost::float64_t value_in_mV) const;
        bool operator==(const Units::millivolt_quantity value_in_mV) const;

    private:
        Units::millivolt_quantity m_ORP;
    };

}
// namespace AqualinkAutomate::Kernel
