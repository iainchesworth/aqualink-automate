#pragma once

#include <cstdint>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	enum class ChlorinatorStatus : uint8_t
	{
		Off,
		Running,
		Unknown
	};

	class Chlorinator : public AuxillaryBaseWithStatus<ChlorinatorStatus>
	{
	public:
		Chlorinator(const std::string& label);
		Chlorinator(const std::string& label, const ChlorinatorStatus status);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		using AuxillaryBaseWithStatus<ChlorinatorStatus>::Status;
		virtual void Status(const ChlorinatorStatus chlorinator_status) override;

	public:
		Chlorinator& operator=(const Utility::AuxillaryState& aux_state);
		Chlorinator& operator=(const ChlorinatorStatus chlorinator_status);

	public:
		static ChlorinatorStatus ConvertToChlorinatorStatus(AuxillaryStatuses aux_states);
	};

}
// namespace AqualinkAutomate::Kernel
