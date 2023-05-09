#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "interfaces/iequipment.h"
#include "jandy/generator/jandy_message_generator.h"
#include "protocol/protocol_handler.h"

// Forward declarations
namespace AqualinkAutomate::HTTP
{
	class WebRoute_JandyEquipment;
}
// namespace AqualinkAutomate::HTTP

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Equipment
{

	class JandyEquipment : public Interfaces::IEquipment<Protocol::ProtocolHandler<Generators::JandyMessageGenerator>>
	{
		friend class AqualinkAutomate::HTTP::WebRoute_JandyEquipment;

	public:
		JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler);

	private:
		auto IsDeviceRegistered(Interfaces::IDevice::DeviceId device_id);

	private:
		void StopAndCleanUp() override;

	private:
		boost::asio::io_context& m_IOContext;
		std::vector<std::shared_ptr<Interfaces::IDevice>> m_Devices;
		std::unordered_set<uint8_t> m_IdentifiedDeviceIds;
		std::unordered_map<Messages::JandyMessageIds, uint32_t> m_MessageStats;
	};

}
// namespace AqualinkAutomate::Equipment
