#pragma once

namespace AqualinkAutomate::Devices::Capabilities
{

	// The action a caller wants applied to a logical device. Mirrors
	// Interfaces::ICommandDispatcher::DeviceAction so the dispatcher can translate
	// without the capability layer depending on the core command interface.
	enum class ActuationAction
	{
		On,
		Off,
		Toggle
	};

	// The outcome of asking a controller to actuate something. The dispatcher maps
	// this back onto Interfaces::ICommandDispatcher::CommandResult.
	//
	//   Accepted      - the controller queued the command for emission on the wire.
	//   NotSupported  - this controller cannot perform the requested action.
	//   InvalidValue  - the request was well-formed but the value was out of range.
	//   MappingFailed - the controller is capable in general but could not map THIS
	//                   particular device/request onto a wire command.
	enum class ActuationResult
	{
		Accepted,
		NotSupported,
		InvalidValue,
		MappingFailed
	};

	// When more than one connected controller advertises the same actuation
	// capability (e.g. a Serial Adapter AND an emulated OneTouch can both toggle
	// equipment), the dispatcher prefers the numerically-lowest priority. A direct
	// command channel (Serial Adapter) is faster and more reliable than menu
	// navigation (OneTouch/PDA), so it ranks higher.
	enum class ActuationPriority
	{
		High = 0,    // direct command channel (e.g. Serial Adapter)
		Low = 1,     // menu-navigation channel (e.g. OneTouch)
		Lowest = 2   // last-resort channel
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
