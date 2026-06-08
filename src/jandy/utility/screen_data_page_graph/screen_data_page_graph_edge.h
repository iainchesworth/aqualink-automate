#pragma once

#include <cstdint>

namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
{

	using EdgeId = uint32_t;

	// Canonical navigation key command stored on a graph edge.
	//
	// Previously the edge held a type-erased std::any so that each device's
	// own KeyCommands enum could be stuffed in and any_cast back out.  In
	// practice every device's key-command enum shares the identical uint8_t
	// wire semantics (Select/LineDown/LineUp/Back/...), so a single concrete
	// enum removes the type erasure (and the bad_any_cast handling in
	// Scrapeable) without losing any expressiveness.  NoKeyCommand is the
	// "no value" sentinel that replaces the old std::any::has_value() check.
	enum class KeyCommand : uint8_t
	{
		NoKeyCommand = 0x00,
		PageDown_Or_Select1 = 0x01,
		Back_Or_Select2 = 0x02,
		PageUp_Or_Select3 = 0x03,
		Select = 0x04,
		LineDown = 0x05,
		LineUp = 0x06,
		Unknown = 0xFF
	};

	struct Edge
	{
		EdgeId id{ 0 };
		KeyCommand key_command{ KeyCommand::NoKeyCommand };
	};

}
// using namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
