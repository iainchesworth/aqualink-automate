#pragma once

#include <string>
#include <cstdint>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	enum class HeaterStatus : uint8_t
	{
		Off = 0x00,
		Heating = 0x01,
		Enabled = 0x04,
		NotInstalled,
		Unknown
	};

	class Heater : public AuxillaryBaseWithStatus<HeaterStatus>
	{
	public:
		Heater(const std::string& label);
		Heater(const std::string& label, const HeaterStatus status);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:


	public:
		using AuxillaryBaseWithStatus<HeaterStatus>::Status;
		virtual void Status(const HeaterStatus heater_status) override;

	public:
		Heater& operator=(const Utility::AuxillaryState& aux_state);
		Heater& operator=(const HeaterStatus heater_status);

	public:
		static HeaterStatus ConvertToHeaterStatus(AuxillaryStatuses aux_status);
	};
	

}
// namespace AqualinkAutomate::Kernel
