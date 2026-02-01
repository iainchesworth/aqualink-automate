#pragma once

namespace AqualinkAutomate::Jandy::Protocol
{

	/// Register the Jandy message generator with the core protocol handler.
	/// This should be called during application initialization before starting
	/// the protocol handler.
	void RegisterMessageGenerator();

	/// Unregister the Jandy message generator.
	/// This is typically called during shutdown.
	void UnregisterMessageGenerator();

}
// namespace AqualinkAutomate::Jandy::Protocol

