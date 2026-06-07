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
		EmulatedDeviceUpdate,

		Unknown
	};

}
// namespace AqualinkAutomate::HTTP
