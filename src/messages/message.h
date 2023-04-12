#pragma once

#include "serialization/serializable.h"

namespace AqualinkAutomate::Messages
{

	class Message : public AqualinkAutomate::Serialization::Serializable
	{
	public:
		bool operator==(const Message& other) const;
	};

}
// namespace AqualinkAutomate::Messages
