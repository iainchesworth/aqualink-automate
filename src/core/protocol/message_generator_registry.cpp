#include <algorithm>

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
		std::lock_guard lock(m_Mutex);
		m_Generators.push_back({ std::move(generator), priority });

		// Sort by priority (lower values = higher priority)
		std::sort(m_Generators.begin(), m_Generators.end(),
			[](const GeneratorEntry& a, const GeneratorEntry& b) {
				return a.priority < b.priority;
			});

		LogDebug(Channel::Protocol, std::format("Registered message generator (priority: {}, total: {})", priority, m_Generators.size()));
	}

	void MessageGeneratorRegistry::Clear()
	{
		std::lock_guard lock(m_Mutex);
		m_Generators.clear();
		LogDebug(Channel::Protocol, "Cleared all message generators");
	}

	ExpectedProtocolMessage MessageGeneratorRegistry::GenerateMessage(boost::circular_buffer<uint8_t>& buffer)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MessageGeneratorRegistry::GenerateMessage", std::source_location::current());
		zone->Value(buffer.size());

		std::lock_guard lock(m_Mutex);

		if (m_Generators.empty())
		{
			LogDebug(Channel::Protocol, "No message generators registered");
			return std::unexpected(make_error_code(ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator));
		}

		// Try each generator in priority order
		for (const auto& entry : m_Generators)
		{
			auto result = entry.generator(buffer);

			// If successful or if the error indicates the buffer was consumed, return
			if (result.has_value())
			{
				return result;
			}

			// If the generator returned WaitingForMoreData, continue to next generator
			// (another protocol might have enough data)
			if (result.error().value() != ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData)
			{
				// For other errors (like InvalidPacketFormat), return immediately
				// This means this generator tried to parse but failed
				return result;
			}
		}

		// All generators are waiting for more data
		return std::unexpected(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
	}

	bool MessageGeneratorRegistry::HasGenerators() const
	{
		std::lock_guard lock(m_Mutex);
		return !m_Generators.empty();
	}

	std::size_t MessageGeneratorRegistry::GeneratorCount() const
	{
		std::lock_guard lock(m_Mutex);
		return m_Generators.size();
	}

}
// namespace AqualinkAutomate::Protocol

