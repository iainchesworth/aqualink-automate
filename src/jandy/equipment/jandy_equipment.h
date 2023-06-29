#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "interfaces/iequipment.h"
#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/generator/jandy_rawdata_generator.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/utility/signalling_stats_counter.h"
#include "protocol/protocol_handler.h"

namespace AqualinkAutomate::Equipment
{

	class JandyEquipment : public Interfaces::IEquipment
	{
	public:
		JandyEquipment(boost::asio::io_context& io_context, Config::JandyConfig& config);
		virtual ~JandyEquipment();

	private:
		auto IsDeviceRegistered(const Devices::JandyDeviceType& device_id);
		void IdentifyAndAddDevice(const Messages::JandyMessage& message);

	private:
		void DisplayUnknownMessages(const Messages::JandyMessage& message);

	public:
		bool AddEmulatedDevice(std::unique_ptr<Devices::JandyDevice> device);

	public:
		using ConfigType = Config::JandyConfig;
		using StatsType = Utility::SignallingStatsCounter<Messages::JandyMessageIds>;

		const ConfigType& Config() const;
		const StatsType& MessageStats() const;

	private:
		boost::asio::io_context& m_IOContext;
		std::vector<std::unique_ptr<Devices::JandyDevice>> m_Devices;
		std::unordered_set<Devices::JandyDeviceId> m_IdentifiedDeviceIds;
		StatsType m_MessageStats;
		ConfigType& m_Config;
	};

}
// namespace AqualinkAutomate::Equipment
