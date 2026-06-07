#pragma once

namespace AqualinkAutomate::Interfaces
{

	// Implemented by device capabilities that can be run in an emulated
	// (software-driven) mode. Lives in the core interface layer so that
	// core consumers (e.g. HTTP diagnostics routes) can query emulation
	// state polymorphically without depending on the jandy device layer.
	class IEmulatedDevice
	{
	public:
		virtual ~IEmulatedDevice() = default;
		virtual bool IsEmulated() const = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
