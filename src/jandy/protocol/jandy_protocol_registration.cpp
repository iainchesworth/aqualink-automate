#include "errors/protocol_errors.h"
#include "generator/jandy_message_generator.h"
#include "logging/logging.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_message_types.h"
#include "protocol/jandy_protocol_registration.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Jandy::Protocol
{

	void RegisterMessageGenerator()
	{
		LogInfo(Channel::Protocol, "Registering Jandy message generator with protocol handler");

		// Create an adapter that converts Jandy message types to generic protocol message types
		auto jandy_generator = [](boost::circular_buffer<uint8_t>& buffer) -> AqualinkAutomate::Protocol::ExpectedProtocolMessage
		{
			auto result = Generators::GenerateMessageFromRawData(buffer);

			if (result.has_value())
			{
				// Wrap the Jandy message in a ProtocolMessageWrapper for type-erased handling
				// The wrapper stores the shared_ptr and provides access to the signal interface
				return AqualinkAutomate::Protocol::ProtocolMessageWrapper(result.value());
			}
			else
			{
				return std::unexpected(result.error());
			}
		};

		// Register with priority 1 so the (non-destructive) Pentair generator at
		// priority 0 gets first refusal on each buffer.  The Jandy generator
		// clears the whole buffer when it finds no DLE/STX start, which would
		// otherwise discard an in-flight Pentair frame; letting Pentair run first
		// (it leaves non-Pentair buffers untouched) keeps both protocols working
		// on a shared stream.  When Jandy is the only registered protocol this
		// priority is immaterial — its behaviour is unchanged.
		AqualinkAutomate::Protocol::MessageGeneratorRegistry::Instance().Register(jandy_generator, 1);
	}

	void UnregisterMessageGenerator()
	{
		LogInfo(Channel::Protocol, "Unregistering Jandy message generator");
		// Note: Currently the registry doesn't support individual unregistration
		// If needed, this could be added by returning a registration handle
	}

}
// namespace AqualinkAutomate::Jandy::Protocol

