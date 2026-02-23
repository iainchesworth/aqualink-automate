#pragma once

namespace AqualinkAutomate::Serial::RFC2217
{

	enum class State
	{
		DATA,
		IAC,
		COMMAND,
		SUBNEG,
		SUBNEG_IAC
	};

}
// namespace AqualinkAutomate::Serial::RFC2217
