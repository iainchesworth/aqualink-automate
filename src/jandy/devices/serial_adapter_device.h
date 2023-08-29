#pragma once

#include <chrono>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "jandy/auxillaries/jandy_auxillary_id.h"
#include "jandy/auxillaries/jandy_auxillary_status.h"
#include "jandy/devices/jandy_controller.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/capabilities/emulated.h"
#include "jandy/devices/capabilities/restartable.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_ready.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_status.h"
#include "kernel/hub_locator.h"
#include "kernel/temperature.h"
#include "profiling/profiling.h"

namespace AqualinkAutomate::Devices
{

	class SerialAdapterDevice : public JandyController, public Capabilities::Restartable, public Capabilities::Emulated
	{
		inline static const std::chrono::seconds SERIALADAPTER_TIMEOUT_DURATION{ std::chrono::seconds(30) };
		inline static const double SERIALADAPTER_INVALID_TEMPERATURE_CUTOFF{ -17.0f };

	public:
		SerialAdapterDevice(boost::asio::io_context& io_context, std::shared_ptr<Devices::JandyDeviceType> device_id, Kernel::HubLocator& hub_locator, bool is_emulated);
		virtual ~SerialAdapterDevice();

	private:
		virtual void ProcessControllerUpdates() override;

	private:
		virtual void WatchdogTimeoutOccurred() override;

	private:
		void Slot_SerialAdapter_Ack(const Messages::JandyMessage_Ack& msg);
		void Slot_SerialAdapter_DevReady(const Messages::SerialAdapterMessage_DevReady& msg);
		void Slot_SerialAdapter_DevStatus(const Messages::SerialAdapterMessage_DevStatus& msg);
		void Slot_SerialAdapter_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_SerialAdapter_Status(const Messages::JandyMessage_Status& msg);
		void Slot_SerialAdapter_Unknown(const Messages::JandyMessage_Unknown& msg);

	private:
		void Command_SerialAdapter_Model(const uint16_t model_type);
		void Command_SerialAdapter_OpMode(const Messages::SerialAdapter_SCS_OpModes& op_mode);
		void Command_SerialAdapter_Options(const Messages::SerialAdapter_SCS_Options& options);
		void Command_SerialAdapter_BatteryCondition(const Messages::SerialAdapter_SCS_BatteryCondition& battery_condition);
		void Command_SerialAdapter_AirTemperature(const Kernel::Temperature& temperature);
		void Command_SerialAdapter_PoolTemperature(const Kernel::Temperature& temperature);
		void Command_SerialAdapter_SpaTemperature(const Kernel::Temperature& temperature);
		void Command_SerialAdapter_SolarTemperature(const Kernel::Temperature& temperature);
		void Command_SerialAdapter_AuxillaryStatus(const Auxillaries::JandyAuxillaryIds& aux_id, const Auxillaries::JandyAuxillaryStatuses& status);

	private:
		std::vector<Messages::SerialAdapter_StatusTypes> m_StatusTypesCollection;
		std::vector<Messages::SerialAdapter_StatusTypes>::const_iterator m_StatusTypesCollectionIter;

	private:
		bool m_StatusMessageReceived;

	private:
		Types::ProfilingUnitTypePtr m_ProfilingDomain;
	};

}
// namespace AqualinkAutomate::Devices
