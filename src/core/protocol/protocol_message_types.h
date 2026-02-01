#pragma once

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <boost/system/error_code.hpp>

#include "interfaces/imessagesignal_recv.h"

namespace AqualinkAutomate::Protocol
{

	/// Wrapper for protocol messages that provides type-erased access to the message
	/// and its signal interface. This allows different protocol implementations
	/// (Jandy, Pentair, etc.) to use their own message types.
	class ProtocolMessageWrapper
	{
	public:
		ProtocolMessageWrapper() = default;

		template<typename MessageType>
		explicit ProtocolMessageWrapper(std::shared_ptr<MessageType> message)
			: m_Message(message)
			, m_SignalGetter([message]() -> Interfaces::IMessageSignalRecvBase* {
				return dynamic_cast<Interfaces::IMessageSignalRecvBase*>(message.get());
			})
		{
		}

		/// Check if the wrapper contains a valid message.
		explicit operator bool() const { return m_Message != nullptr; }

		/// Get the signal interface for the message (may return nullptr).
		Interfaces::IMessageSignalRecvBase* GetSignalInterface() const
		{
			return m_SignalGetter ? m_SignalGetter() : nullptr;
		}

		/// Get the raw message pointer for type-specific handling.
		template<typename MessageType>
		std::shared_ptr<MessageType> GetMessage() const
		{
			return std::static_pointer_cast<MessageType>(m_Message);
		}

	private:
		std::shared_ptr<void> m_Message;
		std::function<Interfaces::IMessageSignalRecvBase*()> m_SignalGetter;
	};

	/// Shared pointer to a protocol message wrapper.
	using ProtocolMessagePtr = ProtocolMessageWrapper;

	/// Error code type for protocol operations.
	using ProtocolErrorCode = boost::system::error_code;

	/// Expected type for protocol message results.
	using ExpectedProtocolMessage = std::expected<ProtocolMessagePtr, ProtocolErrorCode>;

	/// Interface for a protocol message generator.
	/// Implementations parse raw serial data and generate protocol-specific messages.
	class IMessageGenerator
	{
	public:
		virtual ~IMessageGenerator() = default;

		/// Attempt to generate a message from the given buffer.
		/// May consume bytes from the buffer if a message is found.
		/// Returns an expected containing the message or an error code.
		virtual ExpectedProtocolMessage GenerateFromBuffer(std::vector<uint8_t>& buffer) = 0;

		/// Get the name of the protocol this generator handles.
		virtual std::string ProtocolName() const = 0;
	};

	/// Shared pointer to a message generator.
	using MessageGeneratorPtr = std::shared_ptr<IMessageGenerator>;

}
// namespace AqualinkAutomate::Protocol

