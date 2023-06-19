#pragma once

#include <condition_variable>
#include <mutex>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>

namespace AqualinkAutomate::Interfaces
{
	class IEquipment
	{
	public:
		IEquipment(boost::asio::io_context& io_context) :
			m_IsStopping(false),
			m_StoppingMutex(),
			m_StoppingCV()
		{
		}

		virtual ~IEquipment()
		{
		}

	private:
		bool m_IsStopping;
		std::mutex m_StoppingMutex;
		std::condition_variable m_StoppingCV;
	};
}
// namespace AqualinkAutomate::Interfaces
