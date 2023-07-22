#pragma once

#include <string>
#include <cstdint>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	class Heater : public AuxillaryBaseWithStatus<HeaterStatuses>
	{
	public:
		Heater(const std::string& label);
		Heater(const std::string& label, const HeaterStatuses status);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:


	public:
		using AuxillaryBaseWithStatus<HeaterStatuses>::Status;
		virtual void Status(const HeaterStatuses heater_status) override;

	public:
		Heater& operator=(const Utility::AuxillaryState& aux_state);
		Heater& operator=(const HeaterStatuses heater_status);

	public:
		static HeaterStatuses ConvertToHeaterStatuses(AuxillaryStatuses aux_status);

	private:
		const boost::uuids::uuid m_Id;
	};
	
}
// namespace AqualinkAutomate::Kernel
