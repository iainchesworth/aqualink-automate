#pragma once

namespace AqualinkAutomate::HTTP
{

	enum class WebSocket_EventTypes
	{
		ChemistryUpdate,
		StatisticsUpdate,
		TemperatureUpdate,

		SystemStatusChange,
		SystemStateUpdate,
		ButtonStateChange,

		AlertTransition,

		Unknown
	};

}
// namespace AqualinkAutomate::HTTP
