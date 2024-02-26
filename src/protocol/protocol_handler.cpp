#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	ProtocolHandler::ProtocolHandler(Types::ExecutorType executor, Serial::SerialPort& serial_port) :
		m_SerialPort(serial_port),
		m_ProfilingDomain(std::move(Factory::ProfilingUnitFactory::Instance().CreateDomain("ProtocolHandler"))),
		m_Executor(std::move(executor))
	{
		m_ProfilingDomain->Start();
	}

	ProtocolHandler::~ProtocolHandler()
	{
		Stop();

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
		boost::asio::post(m_Executor, [&]() -> void { Step(); });
	}

	void ProtocolHandler::Stop()
	{
		boost::system::error_code ec;

		if (m_SerialPort.cancel(ec); ec.failed())
		{
			LogDebug(Channel::Protocol, std::format("Failed to cancel outstanding serial port asynchronous actions.  Error was -> {}", ec.message()));
		}
	}

	void ProtocolHandler::Step()
	{
		auto profiling_frame = Factory::ProfilingUnitFactory::Instance().CreateFrame("ProtocolHandler -> Frame"); 

		bool continue_processing = true;
		
		profiling_frame->Start();

		if ((!m_SerialData_Outgoing.empty()) && (continue_processing = HandleWrite()); !continue_processing)
		{
			LogTrace(Channel::Protocol, "Completed HandleWrite but an error was returned; halting processing.");
		}
		else if (continue_processing = HandleRead(); !continue_processing)
		{
			LogTrace(Channel::Protocol, "Completed HandleRead but an error was returned; halting processing.");
		}
		else
		{
			boost::asio::post(m_Executor, [&]() -> void { Step(); });
		}

		profiling_frame->End();
	}

}
// namespace AqualinkAutomate::Protocol
