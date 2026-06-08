#include "errors/protocol_errors.h"
#include "generator/pentair_message_generator.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_message_types.h"
#include "protocol/pentair_protocol_registration.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Pentair::Protocol
{

	void RegisterMessageGenerator()
	{
		LogInfo(Channel::Protocol, "Registering Pentair message generator with protocol handler");

		// Adapter: convert the Pentair message result into the type-erased
		// protocol message wrapper the registry/handler operate on.
		auto pentair_generator = [](boost::circular_buffer<uint8_t>& buffer) -> AqualinkAutomate::Protocol::ExpectedProtocolMessage
		{
			auto result = Generators::GenerateMessageFromRawData(buffer);

			if (result.has_value())
			{
				return AqualinkAutomate::Protocol::ProtocolMessageWrapper(result.value());
			}

			return std::unexpected(result.error());
		};

		// Priority 0: the Pentair generator runs BEFORE the Jandy generator
		// (priority 1).  This ordering is required for coexistence: the Pentair
		// generator is non-destructive — it leaves the buffer untouched when no
		// Pentair preamble is present — whereas the Jandy generator clears the
		// whole buffer when it finds no DLE/STX start sequence.  Running Pentair
		// first lets it lift out a complete Pentair frame before Jandy's
		// destructive cleanup could discard it; a buffer that is not Pentair is
		// passed through unchanged for Jandy to handle exactly as before.
		AqualinkAutomate::Protocol::MessageGeneratorRegistry::Instance().Register(pentair_generator, 0);
	}

	void UnregisterMessageGenerator()
	{
		// No-op by design (and currently unreferenced).  The shared
		// MessageGeneratorRegistry only supports wholesale Clear() — which the
		// application bootstrap already invokes during shutdown
		// (aqualink-automate.cpp: MessageGeneratorRegistry::Instance().Clear()) —
		// so there is nothing per-generator to undo here.
		//
		// TODO: if the registry ever gains per-generator removal (returning a
		// handle from Register()), unregister this generator's handle here and have
		// RegisterMessageGenerator() retain it.
		LogInfo(Channel::Protocol, "Unregistering Pentair message generator (no-op: registry is cleared wholesale at shutdown)");
	}

}
// namespace AqualinkAutomate::Pentair::Protocol
