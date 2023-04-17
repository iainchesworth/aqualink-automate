#pragma once

#include <string>
#include <type_traits>

namespace AqualinkAutomate::Interfaces
{
	template<typename MESSAGE_ID>
	class IMessage
	{
	public:
		IMessage(const MESSAGE_ID message_id) : 
			m_MessageId(message_id)
		{
		}

		virtual ~IMessage()
		{
		}

	public:
		MESSAGE_ID MessageId() const
		{
			return m_MessageId;
		}

	public:
		virtual std::string ToString() const = 0;

	public:
		bool operator==(const IMessage& other) const
		{
			bool is_equal = true;

			is_equal &= (std::is_same<decltype(*this), decltype(other)>::value);
			is_equal &= (m_MessageId == other.m_MessageId);

			return is_equal;
		}

	private:
		MESSAGE_ID m_MessageId;
	};

}
// namespace AqualinkAutomate::Interfaces
