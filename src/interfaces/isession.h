#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <boost/beast/http/message_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "concepts/is_one_of.h"
#include "developer/instance_counter.h"
#include "logging/logging.h"
#include "utility/overloaded_variant_visitor.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

	class ISession : public Developer::InstanceCounterImpl<ISession>
	{
	protected:
		static constexpr std::size_t QUEUE_LIMIT = 8;

	public:
		ISession() :
			m_SessionId(boost::uuids::random_generator()())
		{
		}
		
		virtual ~ISession()
		{
		}

	public:
		const boost::uuids::uuid& Id() const
		{
			return m_SessionId;
		}

	public:
		virtual void Stop() = 0;

	protected:
		virtual void DoReadImpl() = 0;
		virtual void DoWriteImpl() = 0;

	public:
		void DoRead()
		{
			DoReadImpl();
		}

		virtual bool DoWrite()
		{
			bool const was_full = (QUEUE_LIMIT == m_ResponseQueue.size());

			DoWriteImpl();

			return was_full;
		};

	protected:
		using ResponseQueueTextElement = std::string;
		using ResponseQueueBinaryElement = std::vector<uint8_t>;
		using ResponseQueueMessageElement = boost::beast::http::message_generator;
		using ResponseQueueElement = std::variant<ResponseQueueTextElement, ResponseQueueBinaryElement, ResponseQueueMessageElement>;
		using ResponseQueueType = std::vector<ResponseQueueElement>;

	public:
		template<typename PAYLOAD>
			requires Concepts::IsOneOf<PAYLOAD, ResponseQueueTextElement, ResponseQueueBinaryElement, ResponseQueueMessageElement>
		void QueueWrite(PAYLOAD&& payload)
		{
			// NOTE: The message is copied into the queued payload (as it's an async operation and 
			// guarantees of lifetimes are required).

			m_ResponseQueue.push_back(std::move(payload));

			if (1 == m_ResponseQueue.size())
			{
				LogTrace(Channel::Web, "Response queue has a queued response in it so commence response write loop");
				DoWrite();
			}
		}

	protected:
		ResponseQueueType m_ResponseQueue;

	private:
		const boost::uuids::uuid m_SessionId;
	};

}
// namespace AqualinkAutomate::Interfaces
