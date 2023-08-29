#pragma once

#include <memory>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "interfaces/istatuspublisher.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/utility/slot_connection_manager.h"

namespace AqualinkAutomate::Devices
{

	class JandyDevice : public Interfaces::IDevice, public Interfaces::IStatusPublisher
	{
	public:
		JandyDevice(std::shared_ptr<Devices::JandyDeviceType> device_id);
		virtual ~JandyDevice();

	public:
		const Devices::JandyDeviceType& DeviceId() const;

	protected:
		Utility::SlotConnectionManager m_SlotManager;
	};

}
// namespace AqualinkAutomate::Devices
