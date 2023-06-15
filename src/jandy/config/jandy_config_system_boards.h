#pragma once

namespace AqualinkAutomate::Config
{

	enum class SystemBoards
	{
		RS4_Only,
		RS6_Only,
		RS8_Only,

		RS2_6_Dual,
		RS2_10_Dual,
		RS2_14_Dual,
		RS2_22_Dual,
		RS2_30_Dual,

		RS4_Combo,
		RS6_Combo,
		RS8_Combo,
		RS12_Combo,
		RS16_Combo,
		RS24_Combo,
		RS32_Combo,

		PD4_Only,
		PD8_Only,
		PD4_Combo,
		PD6_Combo,
		PD8_Combo,

		Unknown
	};

}
// namespace AqualinkAutomate::Config
