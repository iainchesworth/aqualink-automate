#pragma once

namespace AqualinkAutomate::HTTP
{

	enum class WebSocket_EventTypes
	{
		ChemistryUpdate,
		StatisticsUpdate,
		TemperatureUpdate,
		CirculationUpdate,

		SystemStatusChange,
		SystemStateUpdate,
		ButtonStateChange,

		AlertTransition,

		Unknown
	};

}
// namespace AqualinkAutomate::HTTP
