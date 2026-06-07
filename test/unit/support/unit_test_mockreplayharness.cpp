#include <boost/asio/error.hpp>

#include "protocol/message_generator_registry.h"
#include "jandy/protocol/jandy_protocol_registration.h"

#include "support/unit_test_mockreplayharness.h"

namespace AqualinkAutomate::Test
{

	MockReplayHarness::MockReplayHarness()
		: m_HubLocator()
		, m_SerialImplPtr(new Test::TestSerialPortImpl())
		, m_SerialPort(std::make_shared<Serial::SerialPort>(
			std::unique_ptr<Test::TestSerialPortImpl>(m_SerialImplPtr), m_HubLocator))
	{
		// Pull the hubs the HubLocatorInjector created so tests can assert on them.
		m_DataHub = m_HubLocator.Find<Kernel::DataHub>();
		m_EquipmentHub = m_HubLocator.Find<Kernel::EquipmentHub>();
		m_StatisticsHub = m_HubLocator.Find<Kernel::StatisticsHub>();

		// Route queued reads through the deterministic test path (queued bytes /
		// queued errors) rather than the base mock's random-data generator.
		m_SerialImplPtr->EnableTestMode(true);

		// Register the real Jandy message generator so raw bytes are parsed by
		// the exact same code path the application uses.  Clear any generators
		// left over from a previous harness/test first for isolation.
		Protocol::MessageGeneratorRegistry::Instance().Clear();
		Jandy::Protocol::RegisterMessageGenerator();

		// The protocol task is the production driver: it reads from the serial
		// port, runs the generator, and fires message signals to device slots.
		m_ProtocolTask = std::make_shared<Protocol::ProtocolTask>(m_SerialPort, m_StatisticsHub);
	}

	MockReplayHarness::~MockReplayHarness()
	{
		// Tear devices down first so their static-signal slot connections are
		// released before the hubs they reference disappear.
		m_Devices.clear();

		// Drop the task (and its write-signal connections) before the port.
		m_ProtocolTask.reset();

		// Don't leak the Jandy generator into unrelated suites.
		Protocol::MessageGeneratorRegistry::Instance().Clear();

		if (m_SerialImplPtr != nullptr)
		{
			m_SerialImplPtr->Reset();
		}
	}

	void MockReplayHarness::ReplayOne(const std::vector<uint8_t>& frame)
	{
		// Deliver the whole frame in a single read, then a would_block sentinel
		// so ProtocolTask::DrainReads() stops cleanly instead of falling through
		// to the base mock's random-data generator (which never reports "empty").
		m_SerialImplPtr->QueueReadData(frame);
		m_SerialImplPtr->QueueReadData({}, boost::asio::error::would_block);

		// A single Poll drains the queued read (frame + sentinel) and processes
		// every complete packet now sitting in the circular buffer.
		(void)m_ProtocolTask->Poll();
	}

	void MockReplayHarness::Replay(const std::vector<uint8_t>& frame)
	{
		ReplayOne(frame);
	}

	void MockReplayHarness::Replay(const std::vector<std::vector<uint8_t>>& frames)
	{
		for (const auto& frame : frames)
		{
			ReplayOne(frame);
		}
	}

}
// namespace AqualinkAutomate::Test
