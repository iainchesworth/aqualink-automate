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

	class Pump : public AuxillaryBaseWithStatus<PumpStatus>
	{
	public:
		Pump(const std::string& label);
		Pump(const std::string& label, const PumpStatus status);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		using AuxillaryBaseWithStatus<PumpStatus>::Status;
		virtual void Status(const PumpStatus pump_status) override;

	public:
		Pump& operator=(const Utility::AuxillaryState& aux_state);
		Pump& operator=(const PumpStatus pump_status);

	public:
		static PumpStatus ConvertToPumpStatus(AuxillaryStatuses aux_states);

	private:
		const boost::uuids::uuid m_Id;
	};

}
// namespace AqualinkAutomate::Kernel
