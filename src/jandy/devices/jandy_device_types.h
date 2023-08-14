#pragma once

#include <cstdint>
#include <list>
#include <utility>
#include <vector>

#include "interfaces/ideviceidentifier.h"
#include "jandy/devices/jandy_device_id.h"

namespace AqualinkAutomate::Devices
{

	enum class DeviceClasses
	{
		AqualinkMaster,
		RS_Keypad,
		SpaRemote,
		OneTouch,
		LX_Heater,
		IAQ,
		SerialAdapter,
		SWG_Aquarite,
		PC_Dock,
		PDA,
		Pentair_Pump,
		Jandy_VSP,
		Chemlink,
		Unknown
	};

	/*
		enum class Pentair_Pump_DeviceIds : uint8_t
		{
			Pentair_Pump_A = 0x60,
			Pentair_Pump_B = 0x61,
			Pentair_Pump_C = 0x62,
			Pentair_Pump_D = 0x63,
			Pentair_Pump_E = 0x64,
			Pentair_Pump_F = 0x65,
			Pentair_Pump_G = 0x66,
			Pentair_Pump_H = 0x67,
			Pentair_Pump_I = 0x67
		};
	*/

	class JandyDeviceType : public Interfaces::IDeviceIdentifier
	{
		using DeviceId = JandyDeviceId;
		using DeviceClassAndIds = std::pair<DeviceClasses, std::vector<DeviceId>>;
		using DeviceClassAndIdsList = std::list<DeviceClassAndIds>;

		inline static const DeviceClasses UNKNOWN_DEVICE_CLASS{ DeviceClasses::Unknown };
		inline static const DeviceId UNKNOWN_DEVICE_RAWID{ 0xFF };

		const DeviceClassAndIdsList m_KnownDeviceIdsList =
		{
			{DeviceClasses::AqualinkMaster, {0x00, 0x01, 0x02, 0x03}},
			{DeviceClasses::RS_Keypad,		{0x08, 0x09, 0x0A, 0x0B}},
			{DeviceClasses::SpaRemote,		{0x20, 0x21, 0x22, 0x23}},
			{DeviceClasses::OneTouch,		{0x40, 0x41, 0x42, 0x43}},
			{DeviceClasses::LX_Heater,		{0x38, 0x39, 0x3A, 0x3B}},
			{DeviceClasses::IAQ,			{0x30, 0x31, 0x32, 0x33}},
			{DeviceClasses::SerialAdapter,	{0x48, 0x49}},
			{DeviceClasses::SWG_Aquarite,	{0x50, 0x51, 0x52, 0x53}},
			{DeviceClasses::PC_Dock,		{0x58, 0x59, 0x5A, 0x5B}},
			{DeviceClasses::PDA,			{0x60, 0x61, 0x62, 0x63}},
			{DeviceClasses::Jandy_VSP,		{0x78, 0x79, 0x7A, 0x7B}},
			{DeviceClasses::Chemlink,		{0x80, 0x81, 0x82, 0x83}},
			{UNKNOWN_DEVICE_CLASS,			{UNKNOWN_DEVICE_RAWID}}
		};

	public:
		JandyDeviceType();
		JandyDeviceType(DeviceId device_id);

	public:
		JandyDeviceType(const JandyDeviceType& other);
		JandyDeviceType& operator=(const JandyDeviceType& other);
		JandyDeviceType(JandyDeviceType&& other) noexcept;
		JandyDeviceType& operator=(JandyDeviceType&& other) noexcept;

	public:
		bool operator==(const JandyDeviceType& other) const;
		bool operator!=(const JandyDeviceType& other) const;
		virtual bool operator==(const Interfaces::IDeviceIdentifier& other) const final;
		virtual bool operator!=(const Interfaces::IDeviceIdentifier& other) const final;

	public:
		DeviceId operator()() const;

	public:
		DeviceClasses Class() const;
		DeviceId Id() const;

	private:
		DeviceClasses m_DeviceClass;
		DeviceId m_DeviceId;
	};

}
// namespace AqualinkAutomate::Devices
