#pragma once

#include <memory>

#include <boost/signals2.hpp>

namespace AqualinkAutomate::Interfaces
{

	template<typename MESSAGE_TYPE>
	class IPublisher
	{
	private:
		IPublisher() = default;

	public:
		virtual ~IPublisher() = default;

	public:
		using MessageType = MESSAGE_TYPE;
		using SignalRef = const MessageType&;
		using SignalFunc = boost::signals2::signal<void(SignalRef)>;
		using SignalPtr = std::shared_ptr<SignalFunc>;

	public:
		static SignalPtr& GetSignal()
		{
			static SignalPtr signal(std::make_shared<SignalFunc>());
			return signal;
		}
	};

}
// namespace AqualinkAutomate::Interfaces
