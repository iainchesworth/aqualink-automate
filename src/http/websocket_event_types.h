#pragma once

namespace AqualinkAutomate::HTTP
{

	enum class WebSocket_EventTypes
	{
		ChemistryUpdate,
		StatisticsUpdate,
		TemperatureUpdate,

		Ping_KeepAlive,
		Pong_KeepAlive,

		Unknown
	};

}
// namespace AqualinkAutomate::HTTP
