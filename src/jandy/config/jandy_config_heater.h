#pragma once

#include <string>
#include <cstdint>

#include <boost/uuid/uuid.hpp>

#include "jandy/config/jandy_config_auxillary_base.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Config
{

	enum class HeaterStatus : uint8_t
	{
		Off = 0x00,
		Heating = 0x01,
		Enabled = 0x04,
		NotInstalled,
		Unknown
	};

	class Heater : public AuxillaryBaseWithState<HeaterStatus>
	{
	public:
		Heater(const std::string& label);
		Heater(const std::string& label, const HeaterStatus state);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		Heater& operator=(const Utility::AuxillaryState& aux_state);

	public:
		static HeaterStatus ConvertToHeaterStatus(AuxillaryStates aux_states);
	};
	

}
// namespace AqualinkAutomate::Config
