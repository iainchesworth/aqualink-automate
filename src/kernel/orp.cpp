#include "kernel/orp.h"

namespace AqualinkAutomate::Kernel
{

    ORP::ORP(boost::float64_t value_in_mV) :
        m_ORP(value_in_mV * Units::millivolts)
    {
    }

    Units::millivolt_quantity ORP::operator()() const
    {
        return m_ORP;
    }

    ORP& ORP::operator=(Units::millivolt_quantity value_in_mV)
    {
        m_ORP = value_in_mV;
        return *this;
    }

    ORP& ORP::operator=(boost::float64_t value_in_mV)
    {
        m_ORP = value_in_mV * Units::millivolts;
        return *this;
    }

    bool ORP::operator==(const ORP& other) const
    {
        return (m_ORP == other.m_ORP);
    }

    bool ORP::operator==(const boost::float64_t value_in_mV) const
    {
        return (m_ORP == (value_in_mV * Units::millivolts));
    }

    bool ORP::operator==(const Units::millivolt_quantity value_in_mV) const
    {
        return (m_ORP == value_in_mV);
    }

}
// namespace AqualinkAutomate::Kernel
