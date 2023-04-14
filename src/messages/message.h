#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "serialization/serializable.h"

namespace AqualinkAutomate::Messages
{

	class Message : public AqualinkAutomate::Serialization::Serializable
	{
	public:
		Message();
		virtual ~Message();

	public:
		virtual std::string ToString() const = 0;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const = 0;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) = 0;

	public:
		bool operator==(const Message& other) const;
	};

}
// namespace AqualinkAutomate::Messages
