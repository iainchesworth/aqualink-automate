#pragma once

#include <condition_variable>
#include <mutex>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>

namespace AqualinkAutomate::Interfaces
{
	template<typename PROTOCOL_HANDLER>
	class IEquipment
	{
	public:
		typedef PROTOCOL_HANDLER ProtocolHandler;

	public:
		IEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler) :
			m_ProtocolHandler(protocol_handler),
			m_IsStopping(false),
			m_StoppingMutex(),
			m_StoppingCV()
		{
		}

		virtual ~IEquipment()
		{
		}

	public:
		boost::asio::awaitable<void> Run()
		{
			if (m_IsStopping)
			{
				// Already flagged as terminating so just clean-up and return.
			}
			else
			{
				std::unique_lock<std::mutex> lock(m_StoppingMutex);
				while (!m_IsStopping) 
				{
					m_StoppingCV.wait(lock);
				}

				StopAndCleanUp();
			}

			co_return;
		}

	protected:
		virtual void StopAndCleanUp() = 0;

	public:
		void Stop()
		{
			std::lock_guard<std::mutex> lock(m_StoppingMutex);
			m_IsStopping = true;
			m_StoppingCV.notify_all();

		}

	protected:
		ProtocolHandler& m_ProtocolHandler;

	private:
		bool m_IsStopping;
		std::mutex m_StoppingMutex;
		std::condition_variable m_StoppingCV;
	};
}
// namespace AqualinkAutomate::Interfaces
