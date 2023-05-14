#pragma once

#include <format>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

	class IMessageSignalSendBase
	{
	public:
		virtual void Signal_MessageToSend() = 0;
	};

	template<typename MESSAGE_TYPE, typename MESSAGE_PUBLISHER>
	class IMessageSignalSend : public IMessageSignalSendBase
	{
	public:
		virtual ~IMessageSignalSend() = default;

	public:
		void Signal_MessageToSend() final
		{
			if (auto signal_ptr = typename MESSAGE_PUBLISHER::GetSignal(); nullptr == signal_ptr)
			{
				LogTrace(Channel::Messages, "Could not retrieve signal shared_ptr from <MESSAGE_PUBLISHER>::GetSignal()");
			}
			else if (auto signal = signal_ptr.get(); nullptr == signal)
			{
				LogTrace(Channel::Messages, "Could not get underlying pointer (nullptr) from signal shared_ptr returned from <MESSAGE_PUBLISHER>::GetSignal()");
			}
			else if (auto upcast_ptr = dynamic_cast<typename MESSAGE_PUBLISHER::MessageType*>(this); nullptr == upcast_ptr)
			{
				const std::type_info& src_type = typeid(this);
				const std::type_info& dst_type = typeid(typename MESSAGE_PUBLISHER::MessageType*);

				LogDebug(Channel::Messages, std::format("Could not convert from 'this' to the target message pointer type: src -> {}, dst -> {}", src_type.name(), dst_type.name()));
			}
			else
			{
				(*signal)(*upcast_ptr);
			}
		}
	};

}
// namespace AqualinkAutomate::Interfaces
