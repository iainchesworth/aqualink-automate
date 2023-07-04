#pragma once

#include <cstdint>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	enum class PumpStatus : uint8_t
	{
		Off = 0x00,
		Running = 0x01,
		NotInstalled,
		Unknown
	};

	class Pump : public AuxillaryBaseWithState<PumpStatus>
	{
	public:
		Pump(const std::string& label);
		Pump(const std::string& label, const PumpStatus state);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		Pump& operator=(const Utility::AuxillaryState& aux_state);

	public:
		static PumpStatus ConvertToPumpStatus(AuxillaryStates aux_states);
	};


}
// namespace AqualinkAutomate::Kernel
