#include "kernel/flow_rate.h"

namespace AqualinkAutomate::Kernel
{
	
	FlowRate::FlowRate(Units::gallons_per_minute rate_in_gpm) :
		m_FlowRateInLPM(rate_in_gpm)
	{
	}

	FlowRate::FlowRate(Units::liters_per_minute rate_in_lpm) :
		m_FlowRateInLPM(rate_in_lpm)
	{
	}

	Units::gallons_per_minute FlowRate::InGPM() const
	{
		return Units::gallons_per_minute(m_FlowRateInLPM);
	}

	Units::liters_per_minute FlowRate::InLPM() const
	{
		return m_FlowRateInLPM;
	}

	FlowRate FlowRate::ConvertToFlowRateInGPM(double flow_rate_in_gpm)
	{
		using AqualinkAutomate::Units::gpm;
		return FlowRate(flow_rate_in_gpm * gpm);
	}

	FlowRate FlowRate::ConvertToFlowRateInLPM(double flow_rate_in_lpm)
	{
		using AqualinkAutomate::Units::lpm;
		return FlowRate(flow_rate_in_lpm * lpm);
	}

}
// namespace AqualinkAutomate::Kernel
