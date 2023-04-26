#pragma once

#include <cstddef>
#include <span>
#include <unordered_map>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/types/jandy_types.h"

namespace AqualinkAutomate::Factory
{

	typedef Types::JandyMessageTypePtr(*JandyMessageInstanceGenerator)();

	class JandyMessageFactory
	{
	private:
		JandyMessageFactory();
		~JandyMessageFactory() = default;

	public:
		JandyMessageFactory(const JandyMessageFactory&) = delete;
		JandyMessageFactory& operator=(const JandyMessageFactory&) = delete;
		JandyMessageFactory(JandyMessageFactory&&) = delete;
		JandyMessageFactory& operator=(JandyMessageFactory&&) = delete;

	public:
		static JandyMessageFactory& Instance();
		bool Register(const Messages::JandyMessageIds type, const JandyMessageInstanceGenerator generator);

	public:
		Types::JandyExpectedMessageType CreateFromSerialData(const std::span<const std::byte>& message_bytes);

	private:
		std::unordered_map<Messages::JandyMessageIds, JandyMessageInstanceGenerator> m_Generators;
	};

}
// namespace AqualinkAutomate::Factory
