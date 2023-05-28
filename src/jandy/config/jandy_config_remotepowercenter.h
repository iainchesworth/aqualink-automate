#pragma once

#include "jandy/config/jandy_config_auxillary.h"

namespace AqualinkAutomate::Config
{

	enum class RemotePowerCenterIds
	{
		B,
		C,
		D
	};

	class RemotePowerCenter
	{
	public:
		Auxillary Aux1;
		Auxillary Aux2;
		Auxillary Aux3;
		Auxillary Aux4;
		Auxillary Aux5;
		Auxillary Aux6;
		Auxillary Aux7;
		Auxillary Aux8;
	};

}
// namespace AqualinkAutomate::Config
