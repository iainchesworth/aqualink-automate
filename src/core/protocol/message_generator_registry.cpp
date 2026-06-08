#include <algorithm>
#include <format>

#include "errors/message_errors.h"
#include "errors/protocol_errors.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "protocol/message_generator_registry.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Protocol
{

	MessageGeneratorRegistry& MessageGeneratorRegistry::Instance()
	{
		static MessageGeneratorRegistry instance;
		return instance;
	}

	void MessageGeneratorRegistry::Register(MessageGeneratorFunc generator, int priority)
	{
		m_Generators.push_back({ std::move(generator), priority });

		// Sort by priority (lower values = higher priority)
		std::ranges::sort(m_Generators,
			[](const GeneratorEntry& a, const GeneratorEntry& b) {
				return a.priority < b.priority;
			});

		LogDebug(Channel::Protocol, std::format("Registered message generator (priority: {}, total: {})", priority, m_Generators.size()));
	}

	void MessageGeneratorRegistry::Clear()
	{
		m_Generators.clear();
		LogDebug(Channel::Protocol, "Cleared all message generators");
	}

	//=========================================================================
	// CONSUME-OR-DEFER CONTRACT
	//
	// Generators are tried in priority order against the SAME shared buffer.
	// Each call MUST return exactly one of:
	//
	//   * value                  -> a frame was decoded; the generator consumed
	//                               (erased) its bytes from the front.
	//   * WaitingForMoreData     -> "these bytes are NOT mine": the buffer is
	//                               left UNTOUCHED so the NEXT generator may claim
	//                               them.  This is the ONLY code that advances to
	//                               the next generator.
	//   * any other error code   -> "these bytes ARE mine": the generator has
	//                               positively identified the protocol.  The
	//                               registry returns immediately and does NOT try
	//                               another generator (preventing a destructive
	//                               co-generator from clearing an identified-but-
	//                               incomplete frame).  Such a generator EITHER
	//                               consumed >= 1 byte (a definitive failure such
	//                               as ChecksumFailure / OverlappingPackets that
	//                               makes forward progress) OR deferred without
	//                               consuming (DataAvailableToProcess = "my frame,
	//                               still arriving").  The protocol task converts a
	//                               non-buffer-shrinking deferral into a bounded
	//                               "await more data" break (see ProcessMessages),
	//                               so a non-conforming generator cannot spin.
	//
	// Detection-by-preamble means a generator can leave the buffer untouched yet
	// still POSITIVELY claim it (incomplete frame) — hence the claim signal is the
	// error code, NOT the buffer-mutation state.
	//=========================================================================
	ExpectedProtocolMessage MessageGeneratorRegistry::GenerateMessage(boost::circular_buffer<uint8_t>& buffer)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MessageGeneratorRegistry::GenerateMessage", std::source_location::current());
		zone->Value(buffer.size());

		if (m_Generators.empty())
		{
			// An empty registry is a configuration error (no protocol was wired up),
			// not a routine per-message condition.  Surface it at Warning with a
			// distinct code so it is not silently folded into GeneratorFailures.
			LogWarning(Channel::Protocol, "No message generators registered; cannot parse serial data");
			return std::unexpected(make_error_code(ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator));
		}

		const auto deferral_code = make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData);

		// Try each generator in priority order.
		for (const auto& entry : m_Generators)
		{
			auto result = entry.generator(buffer);

			if (result.has_value())
			{
				return result;
			}

			// WaitingForMoreData (category-aware) means "not my protocol" -> try the
			// next generator on the same bytes.  Comparing the whole error_code (value
			// AND category) avoids a cross-category collision with an unrelated 2001.
			if (result.error() != deferral_code)
			{
				// Any other error code is a positive claim on these bytes (decode
				// failure or an identified-but-incomplete frame).  Stop here; do NOT
				// offer the buffer to a co-generator that might destructively clear it.
				return result;
			}
		}

		// Every generator declined the buffer.
		return std::unexpected(deferral_code);
	}

	bool MessageGeneratorRegistry::HasGenerators() const
	{
		return !m_Generators.empty();
	}

	std::size_t MessageGeneratorRegistry::GeneratorCount() const
	{
		return m_Generators.size();
	}

}
// namespace AqualinkAutomate::Protocol
