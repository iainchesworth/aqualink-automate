#pragma once

namespace AqualinkAutomate::HTTP
{

	enum class WebSocket_EventTypes
	{
		ChemistryUpdate,
		StatisticsUpdate,
		TemperatureUpdate,

		SystemStatusChange,

		Unknown
	};

}
// namespace AqualinkAutomate::HTTP
