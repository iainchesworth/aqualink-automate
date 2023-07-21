#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "kernel/auxillary_states.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	class Auxillary : public AuxillaryBaseWithStatus<AuxillaryStatuses>
	{
	public:
		Auxillary(const std::string& label);
		Auxillary(const std::string& label, const AuxillaryStatuses status);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		using AuxillaryBaseWithStatus<AuxillaryStatuses>::Status;
		virtual void Status(const AuxillaryStatuses aux_status) override;

	public:
		Auxillary& operator=(const Utility::AuxillaryState& other);

	private:
		const boost::uuids::uuid m_Id;
	};

}
// namespace AqualinkAutomate::Kernel
