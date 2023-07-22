#pragma once

#include <cstdint>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	class Pump : public AuxillaryBaseWithStatus<PumpStatuses>
	{
	public:
		Pump(const std::string& label);
		Pump(const std::string& label, const PumpStatuses status);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		using AuxillaryBaseWithStatus<PumpStatuses>::Status;
		virtual void Status(const PumpStatuses pump_status) override;

	public:
		Pump& operator=(const Utility::AuxillaryState& aux_state);
		Pump& operator=(const PumpStatuses pump_status);

	public:
		static PumpStatuses ConvertToPumpStatuses(AuxillaryStatuses aux_states);

	private:
		const boost::uuids::uuid m_Id;
	};

}
// namespace AqualinkAutomate::Kernel
