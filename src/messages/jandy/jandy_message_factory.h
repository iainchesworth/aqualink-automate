#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <unordered_map>

#include <boost/system/error_code.hpp>

#include "messages/jandy/jandy_message_generator.h"

namespace AqualinkAutomate::Messages::Jandy
{

	typedef JandyMessageGenerator::MessageType(*JandyMessageInstanceGenerator)();

	class JandyMessageFactory
	{
	private:
		JandyMessageFactory();
		~JandyMessageFactory();

	public:
		JandyMessageFactory(const JandyMessageFactory&) = delete;
		JandyMessageFactory& operator=(const JandyMessageFactory&) = delete;
		JandyMessageFactory(JandyMessageFactory&&) = delete;
		JandyMessageFactory& operator=(JandyMessageFactory&&) = delete;

	public:
		static JandyMessageFactory& Instance();
		bool Register(const JandyMessageTypes type, const JandyMessageInstanceGenerator generator);

	public:
		std::expected<JandyMessageGenerator::MessageType, boost::system::error_code> CreateFromSerialData(const std::span<const std::byte>& message_bytes);

	private:
		std::unordered_map<JandyMessageTypes, JandyMessageInstanceGenerator> m_Generators;
	};

}
// namespace AqualinkAutomate::Messages::Jandy
