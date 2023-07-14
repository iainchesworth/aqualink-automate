#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "kernel/auxillary_states.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Kernel
{

	class Auxillary : public AuxillaryBaseWithState<AuxillaryStates>
	{
	public:
		Auxillary(const std::string& label);
		Auxillary(const std::string& label, const AuxillaryStates state);

	public:
		virtual boost::uuids::uuid Id() const final;

	public:
		Auxillary& operator=(const Utility::AuxillaryState& other);
	};

}
// namespace AqualinkAutomate::Kernel
