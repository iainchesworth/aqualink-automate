#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/preferences_hub.h"
#include "kernel/statistics_hub.h"
#include "protocol/protocol_thread.h"
#include "serial/serial_port.h"

#include "mocks/mock_testserialportimpl.h"
#include "support/unit_test_hublocatorinjector.h"

namespace AqualinkAutomate::Test
{

	//=========================================================================
	// MockReplayHarness
	//
	// A REUSABLE, end-to-end test harness that replays synthetic RS-485 frames
	// (raw bytes) through the FULL Jandy protocol decode stack and lets a test
	// assert the resulting decoded state in the kernel hubs / devices — with no
	// real hardware.
	//
	// Wiring (mirrors the production path in src/aqualink-automate.cpp):
	//
	//   raw bytes
	//     -> Test::TestSerialPortImpl  (QueueReadData injects the mock bytes)
	//     -> Serial::SerialPort
	//     -> Protocol::ProtocolTask::Poll()   (read -> circular buffer)
	//     -> MessageGeneratorRegistry / Jandy generator (frame + checksum parse)
	//     -> message static signal fires
	//     -> device slot (e.g. AquariteDevice::Slot_*)
	//     -> kernel hub state (DataHub salt level, chlorinator traits, ...)
	//
	// Devices connect to parsed messages purely via the per-message static
	// signal (IMessageSignalRecv::GetSignal), so a device only has to be
	// constructed against this harness's HubLocator to take part in decoding.
	//
	// Lifetime / isolation: the harness registers the Jandy message generator
	// in its constructor and clears the global MessageGeneratorRegistry in its
	// destructor, so suites that use it do not leak generators into unrelated
	// tests.  Devices added via AddDevice() are owned by the harness and torn
	// down (releasing their static-signal slot connections) before the hubs.
	//=========================================================================
	class MockReplayHarness
	{
	public:
		// single_read_per_poll is forwarded to the ProtocolTask: when true it reads
		// one serial chunk per Poll() exactly as paced --replay-filename does.  It
		// DEFAULTS TO FALSE (drain everything per poll) so the test suite stays fast
		// and deterministic; only tests that exercise replay's per-frame read bound
		// pass true.  (The pacing *sleep* lives in the application frame loop, not
		// here, so the harness drives Poll() at full speed regardless.)
		explicit MockReplayHarness(bool single_read_per_poll = false);
		~MockReplayHarness();

		MockReplayHarness(const MockReplayHarness&) = delete;
		MockReplayHarness& operator=(const MockReplayHarness&) = delete;
		MockReplayHarness(MockReplayHarness&&) = delete;
		MockReplayHarness& operator=(MockReplayHarness&&) = delete;

	public:
		//---------------------------------------------------------------------
		// Device construction
		//---------------------------------------------------------------------

		// Construct a device of type DEVICE inside the harness, forwarding any
		// extra constructor arguments after the mandatory HubLocator&.  The
		// device registers its message slots on construction and stays alive
		// for the lifetime of the harness.  Returns a reference for assertions.
		template<typename DEVICE, typename... ARGS>
		DEVICE& AddDevice(ARGS&&... args)
		{
			auto device = std::make_shared<DEVICE>(std::forward<ARGS>(args)..., HubLocatorRef());
			DEVICE& ref = *device;
			m_Devices.push_back(std::move(device));
			return ref;
		}

		//---------------------------------------------------------------------
		// Replay
		//---------------------------------------------------------------------

		// Replay a single raw frame (already framed + checksummed bytes) through
		// the protocol stack and pump the task until the bytes are consumed.
		void Replay(const std::vector<uint8_t>& frame);

		// Replay a sequence of raw frames in order.  Each frame is processed in
		// its own read so partial-buffering edge cases are exercised exactly as
		// they would be on the wire.
		void Replay(const std::vector<std::vector<uint8_t>>& frames);

		//---------------------------------------------------------------------
		// Accessors for assertions
		//---------------------------------------------------------------------

		Kernel::HubLocator& HubLocatorRef() { return m_HubLocator; }

		std::shared_ptr<Kernel::DataHub> DataHub() const { return m_DataHub; }
		std::shared_ptr<Kernel::EquipmentHub> EquipmentHub() const { return m_EquipmentHub; }
		std::shared_ptr<Kernel::StatisticsHub> StatisticsHub() const { return m_StatisticsHub; }

		Test::TestSerialPortImpl& SerialImpl() const { return *m_SerialImplPtr; }
		Protocol::ProtocolTask& ProtocolTask() const { return *m_ProtocolTask; }

	private:
		// Queue one frame plus a would_block sentinel, then Poll until the
		// protocol task reports no remaining work.
		void ReplayOne(const std::vector<uint8_t>& frame);

	private:
		HubLocatorInjector m_HubLocator;

		std::shared_ptr<Kernel::DataHub> m_DataHub;
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub;

		// Raw pointer is owned by m_SerialPort (passed in as a unique_ptr); we
		// retain it only to inject read data / inspect statistics.
		Test::TestSerialPortImpl* m_SerialImplPtr;
		std::shared_ptr<Serial::SerialPort> m_SerialPort;

		std::shared_ptr<Protocol::ProtocolTask> m_ProtocolTask;

		std::vector<std::shared_ptr<void>> m_Devices;
	};

}
// namespace AqualinkAutomate::Test
