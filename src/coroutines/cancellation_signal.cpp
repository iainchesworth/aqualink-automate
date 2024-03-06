#include "coroutines/cancellation_signal.h"

namespace AqualinkAutomate::Coroutines
{

	void MultiSlot_CancellationSignal::Emit(boost::asio::cancellation_type_t type)
	{
		std::lock_guard<std::mutex> lock(m_SignalCollectionMutex);
		
		for (auto& signal_ptr : m_SignalCollection)
		{
			signal_ptr->emit(type);
		}
	}

	boost::asio::cancellation_slot MultiSlot_CancellationSignal::Slot()
	{
		std::lock_guard<std::mutex> lock(m_SignalCollectionMutex);

		return m_SignalCollection.emplace_back(std::make_unique<boost::asio::cancellation_signal>())->slot();
	}

}
// namespace AqualinkAutomate::Coroutines
