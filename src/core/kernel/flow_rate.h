#pragma once

#include "types/units_flow_rate.h"

namespace AqualinkAutomate::Kernel
{

	class FlowRate
	{
	public:
		FlowRate(Units::gallons_per_minute rate_in_gpm);
		FlowRate(Units::liters_per_minute rate_in_lpm);

	public:
		Units::gallons_per_minute InGPM() const;
		Units::liters_per_minute InLPM() const;

	public:
		static FlowRate ConvertToFlowRateInGPM(double flow_rate_in_gpm);
		static FlowRate ConvertToFlowRateInLPM(double flow_rate_in_lpm);

	private:
		Units::liters_per_minute m_FlowRateInLPM;
	};

}
// namespace AqualinkAutomate::Kernel
