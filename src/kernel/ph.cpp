#include <algorithm>
#include <cmath>
#include <limits>

#include "kernel/ph.h"

namespace AqualinkAutomate::Kernel
{
	
	pH::pH(boost::float32_t value) :
		m_pH(ClampAndRound(value))
	{
	}

	boost::float32_t pH::operator()() const
	{
		return m_pH;
	}

	pH& pH::operator=(boost::float32_t value)
	{
		m_pH = ClampAndRound(value);
		return *this;
	}

	bool pH::operator==(const pH& other) const
	{
		return (std::fabs(m_pH - other.m_pH) < std::numeric_limits<boost::float32_t>::epsilon());
	}

	boost::float32_t AqualinkAutomate::Kernel::pH::ClampAndRound(boost::float32_t value)
	{
		// Limit the value between 0 and 14
		value = std::clamp(value, MINIMUM_PH_VALUE, MAXIMUM_PH_VALUE);

		// Round to 1 decimal place
		value = std::roundf(value * 10.0) / 10.0;

		return value;
	}

}
// namespace AqualinkAutomate::Kernel
