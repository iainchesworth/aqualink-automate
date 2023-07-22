#pragma once

#include <cstdint>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	class Chlorinator : public AuxillaryBaseWithStatus<ChlorinatorStatuses>
	{
	public:
		Chlorinator(const std::string& label);
		Chlorinator(const std::string& label, const ChlorinatorStatuses status);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		using AuxillaryBaseWithStatus<ChlorinatorStatuses>::Status;
		virtual void Status(const ChlorinatorStatuses chlorinator_status) override;

	public:
		Chlorinator& operator=(const Utility::AuxillaryState& aux_state);
		Chlorinator& operator=(const ChlorinatorStatuses chlorinator_status);

	public:
		static ChlorinatorStatuses ConvertToChlorinatorStatuses(AuxillaryStatuses aux_states);

	private:
		const boost::uuids::uuid m_Id;
	};

}
// namespace AqualinkAutomate::Kernel
