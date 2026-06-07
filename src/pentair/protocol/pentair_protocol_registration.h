#pragma once

namespace AqualinkAutomate::Pentair::Protocol
{

	/// Register the Pentair message generator with the core protocol handler.
	/// Called during application initialisation (after the Jandy generator) so
	/// both protocols can be auto-detected on the same serial stream.
	void RegisterMessageGenerator();

	/// Unregister the Pentair message generator (no-op until the registry grows
	/// per-generator removal support; the registry is cleared wholesale on
	/// shutdown).
	void UnregisterMessageGenerator();

}
// namespace AqualinkAutomate::Pentair::Protocol
