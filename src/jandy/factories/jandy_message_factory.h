#pragma once

#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <unordered_map>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#include "concepts/jandy_message_type.h"
#include "errors/jandy_errors_messages.h"
#include "errors/jandy_errors_protocol.h"
#include "formatters/jandy_device_formatters.h"
#include "formatters/jandy_message_formatters.h"
#include "messages/jandy_message_ids.h"
#include "types/jandy_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Factory
{

	template<Concepts::JandyMessageType MESSAGE_TYPE>
	struct MessageCreator
	{
		[[nodiscard]] static constexpr Types::JandyMessageTypePtr Create() noexcept
		{
			LogTrace(Channel::Messages, std::format("Creating message instance for message"));
			return std::make_shared<MESSAGE_TYPE>();
		}
	};

	template<Concepts::JandyMessageType MESSAGE_TYPE, Messages::JandyMessageIds ID, bool IsHotPath = false>
	struct MessageRegistration
	{
		using MessageType = MESSAGE_TYPE;

		static constexpr Messages::JandyMessageIds id = ID;
		static constexpr bool is_hot_path = IsHotPath;

		[[nodiscard]] constexpr Types::JandyMessageTypePtr Create() const noexcept
		{
			LogTrace(Channel::Messages, std::format("Retrieving message creator for message id: {} (hot path: {})", id, is_hot_path));
			return MessageCreator<MESSAGE_TYPE>::Create();
		}
	};

	struct alignas(64) HotPathEntry 
	{
		Messages::JandyMessageIds id;
		Types::JandyMessageTypePtr(*creator)() noexcept;

		constexpr HotPathEntry() noexcept : 
			id(Messages::JandyMessageIds::Unknown), 
			creator(nullptr)
		{
		}

		template<Concepts::JandyMessageType T>
		constexpr HotPathEntry(Messages::JandyMessageIds msg_id) noexcept : 
			id(msg_id), 
			creator(&MessageCreator<T>::Create) 
		{
		}
	};

	struct HashTableEntry 
	{
		std::optional<Messages::JandyMessageIds> id;
		Types::JandyMessageTypePtr(*creator)() noexcept;

		constexpr HashTableEntry() noexcept :
			id(std::nullopt),
			creator(nullptr)
		{
		}

		template<Concepts::JandyMessageType T>
		constexpr HashTableEntry(Messages::JandyMessageIds msg_id) noexcept : 
			id(msg_id), 
			creator(&MessageCreator<T>::Create)
		{
		}
	};

	template<typename... Registrations>
	consteval auto BuildMessageTables() noexcept
	{
		constexpr std::size_t hot_path_capacity = 4;
		std::array<HotPathEntry, hot_path_capacity> hot_cache{};
		std::array<HashTableEntry, 256> cold_table{};

		std::size_t hot_count = 0;

		auto insert = [&](auto tag)
			{
				using R = typename decltype(tag)::type;
				constexpr bool is_hot_path = R::is_hot_path;
				constexpr auto message_id = R::id;

				if constexpr (is_hot_path)
				{
					if (hot_count < hot_path_capacity)
					{
						auto& e = hot_cache[hot_count++];
						e.id = message_id;
						e.creator = &MessageCreator<typename R::MessageType>::Create;
					}
				}

				auto& ce = cold_table[static_cast<std::size_t>(magic_enum::enum_integer(message_id))];
				ce.id = message_id;
				ce.creator = &MessageCreator<typename R::MessageType>::Create;
			};

		(insert(std::type_identity<Registrations>{}), ...);

		return std::make_pair(hot_cache, cold_table);
	}

	template<typename... Registrations>
	class JandyMessageFactory {
	private:
		static constexpr auto tables = BuildMessageTables<Registrations...>();
		static constexpr auto& hot_cache = tables.first;
		static constexpr auto& cold_table = tables.second;
		static constexpr std::size_t registry_size = sizeof...(Registrations);

		// Count hot messages
		static consteval std::size_t CountHotMessages() noexcept 
		{
			std::size_t count = 0;
			auto check = [&count]<typename R>()
			{
				if constexpr (R::is_hot_path)
				{
					++count;
				}
			};

			(check.template operator() < Registrations > (), ...);
			return count;
		}

		static constexpr std::size_t hot_message_count = CountHotMessages();

	public:
		JandyMessageFactory() = delete;

		[[nodiscard]] static Types::JandyMessageTypePtr CreateMessageFromRaw(Messages::JandyMessageIds id) noexcept
		{
			LogDebug(Channel::Messages, std::format("Creating message type from message id; id -> {}", id));

			// Check hot cache first
			for (std::size_t i = 0; i < hot_message_count; ++i) 
			{
				if (hot_cache[i].id == id) 
				{
					LogTrace(Channel::Messages, std::format("Message type (id: {}) creator in HOT-TABLE", id));
					return hot_cache[i].creator();
				}
			}

			// Check cold table
			const auto& entry = cold_table[static_cast<std::size_t>(magic_enum::enum_integer(id))];
			if (entry.id.has_value()) 
			{
				LogTrace(Channel::Messages, std::format("Message type (id: {}) creator in COLD-TABLE", id));
				return entry.creator();
			}

			LogWarning(Channel::Messages, std::format("Could not find suitable message type creator for type {}", id));
			return nullptr;

		}

		// Runtime creation from serial data
		template <std::ranges::random_access_range MESSAGE_BYTES_RANGE>
			requires std::same_as<std::ranges::range_value_t<MESSAGE_BYTES_RANGE>, uint8_t>
		[[nodiscard]] static Types::JandyExpectedMessageType CreateFromSerialData(const MESSAGE_BYTES_RANGE& message_bytes)
		{
			std::shared_ptr<Messages::JandyMessage> message{ nullptr };
			boost::system::error_code return_value;

			if (2 > std::ranges::size(message_bytes))
			{
				LogDebug(Channel::Messages, "Attempted to generate a message from an invalid packet: data was too short");
				return_value = make_error_code(ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat);
			}
			else
			{
				auto first_it = std::ranges::begin(message_bytes);
				auto type_it = first_it + static_cast<std::ptrdiff_t>(Messages::JandyMessage::Index_MessageType);
				const uint8_t raw_message_type = *type_it;

				const Messages::JandyMessageIds message_type(magic_enum::enum_cast<Messages::JandyMessageIds>(raw_message_type)
					.value_or(Messages::JandyMessageIds::Unknown));

				LogTrace(Channel::Messages, std::format("Generating: Jandy message --> {} (0x{:02x})", magic_enum::enum_name(message_type), static_cast<uint8_t>(message_type)));

				if (Types::JandyMessageTypePtr message = CreateMessageFromRaw(message_type); !message)
				{
					LogDebug(Channel::Messages, "Generating: Failed to create Jandy message; nullptr returned");
					return_value = make_error_code(ErrorCodes::Message_ErrorCodes::Error_GeneratorFailed);
				}
				else if (!message->Deserialize(message_bytes))
				{
					LogDebug(Channel::Messages, "Failed to deserialise serial bytes into generated message");
					return_value = make_error_code(ErrorCodes::Message_ErrorCodes::Error_FailedToDeserialize);
				}
				else
				{
					LogTrace(Channel::Messages, std::format("Message Contents -> {{{}}}", *message));
					return message;
				}
			}

			return std::unexpected<boost::system::error_code>(return_value);
		}

		// Query functions
		[[nodiscard]] static consteval std::size_t RegisteredCount() noexcept 
		{
			return registry_size;
		}

		[[nodiscard]] static consteval std::size_t HotPathCount() noexcept
		{
			return hot_message_count;
		}
	};

}
// namespace AqualinkAutomate::Factory
