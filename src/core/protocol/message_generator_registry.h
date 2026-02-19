#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <vector>

#include <boost/circular_buffer.hpp>

#include "protocol/protocol_message_types.h"

namespace AqualinkAutomate::Protocol
{

	/// Function signature for message generators.
	/// Takes a circular buffer of raw bytes and attempts to extract a message.
	/// Returns an expected containing the message or an error code.
	using MessageGeneratorFunc = std::function<ExpectedProtocolMessage(boost::circular_buffer<uint8_t>&)>;

	/// Registry for protocol message generators.
	/// Allows multiple protocols (Jandy, Pentair, etc.) to register their message generators.
	/// The protocol handler uses this registry to try each generator in sequence.
	class MessageGeneratorRegistry
	{
	public:
		/// Get the singleton instance.
		static MessageGeneratorRegistry& Instance();

		/// Register a message generator with an optional priority (lower = higher priority).
		void Register(MessageGeneratorFunc generator, int priority = 0);

		/// Clear all registered generators.
		void Clear();

		/// Attempt to generate a message using all registered generators.
		/// Tries generators in priority order until one succeeds.
		ExpectedProtocolMessage GenerateMessage(boost::circular_buffer<uint8_t>& buffer);

		/// Check if any generators are registered.
		bool HasGenerators() const;

		/// Get the number of registered generators.
		std::size_t GeneratorCount() const;

	private:
		MessageGeneratorRegistry() = default;
		~MessageGeneratorRegistry() = default;

		MessageGeneratorRegistry(const MessageGeneratorRegistry&) = delete;
		MessageGeneratorRegistry& operator=(const MessageGeneratorRegistry&) = delete;

	private:
		struct GeneratorEntry
		{
			MessageGeneratorFunc generator;
			int priority;
		};

		mutable std::shared_mutex m_Mutex;
		std::vector<GeneratorEntry> m_Generators;
	};

}
// namespace AqualinkAutomate::Protocol

