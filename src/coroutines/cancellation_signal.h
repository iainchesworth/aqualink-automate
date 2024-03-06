#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <boost/asio/cancellation_signal.hpp>

namespace AqualinkAutomate::Coroutines
{

	class MultiSlot_CancellationSignal {
	public:
		MultiSlot_CancellationSignal() = default;
		~MultiSlot_CancellationSignal() = default;

	public:
		void Emit(boost::asio::cancellation_type_t type);
		boost::asio::cancellation_slot Slot();

	private:
		std::vector<std::unique_ptr<boost::asio::cancellation_signal>> m_SignalCollection;
		std::mutex m_SignalCollectionMutex;
	};

}
// namespace AqualinkAutomate::Coroutines
