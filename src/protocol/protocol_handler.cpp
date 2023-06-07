#include "logging/logging.h"
#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	ProtocolHandler::ProtocolHandler(boost::asio::io_context& io_context, Serial::SerialPort& serial_port) :
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

	void ProtocolHandler::Run()
	{
		auto profiling_frame = Factory::ProfilingUnitFactory::Instance().CreateFrame("ProtocolHandler -> Frame");

		bool continue_processing = true;

		do
		{
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
				///TODO
			}

			profiling_frame->End();

		} while (continue_processing);
	}

}
// namespace AqualinkAutomate::Protocol
