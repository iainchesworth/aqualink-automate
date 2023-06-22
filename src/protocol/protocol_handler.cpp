#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	ProtocolHandler::ProtocolHandler(boost::asio::io_context& io_context, Serial::SerialPort& serial_port) :
		m_IOContext(io_context),
		m_SerialPort(serial_port),
		m_SerialData_Incoming(),
		m_SerialData_Outgoing(),
		m_SerialData_IncomingMutex(),
		m_SerialData_OutgoingMutex(),
		m_PublishableMessages(),
		m_ProfilingDomain(std::move(Factory::ProfilingUnitFactory::Instance().CreateDomain("ProtocolHandler")))
	{
		m_ProfilingDomain->Start();
	}

	ProtocolHandler::~ProtocolHandler()
	{
		m_ProfilingDomain->End();
	}

	bool ProtocolHandler::PublishRawData(std::vector<uint8_t>&& raw_data)
	{
		std::lock_guard<std::mutex> lock(m_SerialData_OutgoingMutex);
		std::move(raw_data.begin(), raw_data.end(), std::back_inserter(m_SerialData_Outgoing));

		return true;
	}

	void ProtocolHandler::Run()
	{
		m_IOContext.post([&]() -> void { Step(); });
	}

	void ProtocolHandler::Step()
	{
		auto profiling_frame = Factory::ProfilingUnitFactory::Instance().CreateFrame("ProtocolHandler -> Frame"); 

		bool continue_processing = true;
		
		profiling_frame->Start();

		if ((0 < m_SerialData_Outgoing.size()) && (continue_processing = HandleWrite()); !continue_processing)
		{
			LogTrace(Channel::Protocol, "Completed HandleWrite but an error was returned; halting processing.");
		}
		else if (continue_processing = HandleRead(); !continue_processing)
		{
			LogTrace(Channel::Protocol, "Completed HandleRead but an error was returned; halting processing.");
		}
		else
		{
			m_IOContext.post([&]() -> void { Step(); });
		}

		profiling_frame->End();
	}

}
// namespace AqualinkAutomate::Protocol
