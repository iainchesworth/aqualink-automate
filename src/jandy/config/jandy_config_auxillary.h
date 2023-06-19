#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

#include "jandy/config/jandy_config_auxillary_base.h"
#include "jandy/config/jandy_config_auxillary_states.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

namespace AqualinkAutomate::Config
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
// namespace AqualinkAutomate::Config
