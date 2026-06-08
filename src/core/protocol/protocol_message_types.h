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
			// Resolve the signal interface ONCE here (a single dynamic_cast at
			// construction), storing the non-owning raw pointer.  m_Message keeps
			// the object alive, so this pointer stays valid for the wrapper's
			// lifetime.  This drops the previous per-message std::function alloc
			// (capturing a shared_ptr) and the second dynamic_cast on the dispatch
			// hot path entirely.
			, m_SignalInterface(dynamic_cast<Interfaces::IMessageSignalRecvBase*>(message.get()))
		{
		}

		/// Check if the wrapper contains a valid message.
		explicit operator bool() const { return m_Message != nullptr; }

		/// Get the signal interface for the message (may return nullptr).
		Interfaces::IMessageSignalRecvBase* GetSignalInterface() const
		{
			return m_SignalInterface;
		}

		/// Get the raw message pointer for type-specific handling.
		template<typename MessageType>
		std::shared_ptr<MessageType> GetMessage() const
		{
			return std::static_pointer_cast<MessageType>(m_Message);
		}

	private:
		std::shared_ptr<void> m_Message;
		// Non-owning view into m_Message resolved once at construction (nullptr if
		// the message type does not implement the signal interface).
		Interfaces::IMessageSignalRecvBase* m_SignalInterface{ nullptr };
	};

	/// Shared pointer to a protocol message wrapper.
	using ProtocolMessagePtr = ProtocolMessageWrapper;

	/// Error code type for protocol operations.
	using ProtocolErrorCode = boost::system::error_code;

	/// Expected type for protocol message results.
	using ExpectedProtocolMessage = std::expected<ProtocolMessagePtr, ProtocolErrorCode>;

}
// namespace AqualinkAutomate::Protocol

