#pragma once 

#include <format>
#include <memory>
#include <type_traits>

#include <boost/signals2.hpp>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

	class IMessageSignalRecvBase
	{
	public:
		virtual void Signal_MessageWasReceived() = 0;
	};

	template<typename MESSAGE_TYPE>
	class IMessageSignalRecv : public IMessageSignalRecvBase
	{
	public:
		virtual ~IMessageSignalRecv() = default;

	public:
		using SignalRef = const MESSAGE_TYPE&;
		using SignalFunc = boost::signals2::signal<void(SignalRef)>;
		using SignalPtr = std::shared_ptr<SignalFunc>;

		static SignalPtr& GetSignal()
		{
			static SignalPtr signal(std::make_shared<SignalFunc>());
			return signal;
		}

	public:
		void Signal_MessageWasReceived() final
		{
			if (auto signal_ptr = GetSignal(); nullptr == signal_ptr)
			{
				LogTrace(Channel::Messages, "Could not retrieve signal shared_ptr from IMessageSignalRecv::GetSignal()");
			}
			else if (auto signal = signal_ptr.get(); nullptr == signal)
			{
				LogTrace(Channel::Messages, "Could not get underlying pointer (nullptr) from signal shared_ptr returned from IMessageSignalRecv::GetSignal()");
			}
			else if (auto upcast_ptr = dynamic_cast<MESSAGE_TYPE *>(this); nullptr == upcast_ptr)
			{
				const std::type_info& src_type = typeid(this);
				const std::type_info& dst_type = typeid(MESSAGE_TYPE*);

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
