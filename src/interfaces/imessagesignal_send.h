#pragma once

#include <format>
#include <memory>
#include <type_traits>

#include <boost/signals2.hpp>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

	class IMessageSignalSendBase
	{
	public:
		virtual void Signal_MessageToSend() = 0;
	};

	template<typename MESSAGE_TYPE>
	class IMessageSignalSend : public IMessageSignalSendBase
	{
	public:
		virtual ~IMessageSignalSend() = default;

	public:
		using PublisherRef = const MESSAGE_TYPE&;
		using PublisherFunc = boost::signals2::signal<void(PublisherRef)>;
		using PublisherPtr = std::shared_ptr<PublisherFunc>;

		static PublisherPtr& GetPublisher()
		{
			static PublisherPtr publisher(std::make_shared<PublisherFunc>());
			return publisher;
		}

	public:
		void Signal_MessageToSend() final
		{
			if (auto publisher_ptr = GetPublisher(); nullptr == publisher_ptr)
			{
				LogTrace(Channel::Messages, "Could not retrieve signal shared_ptr from IMessageSignalSend::GetPublisher()");
			}
			else if (auto publisher = publisher_ptr.get(); nullptr == publisher)
			{
				LogTrace(Channel::Messages, "Could not get underlying pointer (nullptr) from publisher shared_ptr returned from IMessageSignalSend::GetPublisher()");
			}
			else if (auto upcast_ptr = dynamic_cast<MESSAGE_TYPE *>(this); nullptr == upcast_ptr)
			{
				const std::type_info& src_type = typeid(this);
				const std::type_info& dst_type = typeid(MESSAGE_TYPE*);

				LogDebug(Channel::Messages, std::format("Could not convert from 'this' to the target message pointer type: src -> {}, dst -> {}", src_type.name(), dst_type.name()));
			}
			else
			{
				(*publisher)(*upcast_ptr);
			}
		}
	};

}
// namespace AqualinkAutomate::Interfaces
