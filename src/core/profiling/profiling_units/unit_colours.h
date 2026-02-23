#pragma once

#include <cstdint>

namespace AqualinkAutomate::Profiling
{

	enum class UnitColours : uint32_t
	{
		NotSpecified = 0x00000000,
		Black = 0x00000001, // Note that "0" is reserved for not specified so use a little offset.
		White = 0x00FFFFFF,
		Red = 0x00FF0000,
		Green = 0x0000FF00,
		Blue = 0x000000FF,
		Yellow = 0x00FFFF00,
		Magenta = 0x00FF00FF,
		Cyan = 0x0000FFFF,
		Orange = 0x00FFA500,
		Purple = 0x00800080,
		Brown = 0x00A52A2A,
		Lime = 0x00BFFF00,
		Pink = 0x00FFC0CB,
		Lavender = 0x00E6E6FA,
		Olive = 0x00808000,
		Coral = 0x00FF7F50,
		Teal = 0x00808080,
		Indigo = 0x004B0082,
		Maroon = 0x00800000,
		Navy = 0x00000080,
		Gold = 0x00FFD700,
		Silver = 0x00C0C0C0,
		Plum = 0x00DDA0DD,
		Sienna = 0x00A0522D,
		Salmon = 0x00FA8072,
		Khaki = 0x00F0E68C,
		Slate = 0x00708090,
		Violet = 0x00EE82EE
	};

}
// namespace AqualinkAutomate::Profiling
