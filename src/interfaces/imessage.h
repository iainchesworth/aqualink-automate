#pragma once

#include <cstddef>
#include <string>
#include <span>

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

	public:
		virtual std::string ToString() const = 0;

	public:
		bool operator==(const IMessage& other) const
		{
			return std::is_same<decltype(*this), decltype(other)>::value;
		}

	private:
		MESSAGE_ID m_MessageId;
	};

}
// namespace AqualinkAutomate::Interfaces
