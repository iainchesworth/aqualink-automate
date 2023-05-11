#pragma once

#include "interfaces/igenerator.h"
#include "interfaces/iserialport.h"
#include "jandy/types/jandy_types.h"

namespace AqualinkAutomate::Generators
{

	class JandyRawDataGenerator : public Interfaces::IGenerator_MessageToRawData<Interfaces::ISerialPort::DataType, Types::JandyMessageTypePtr>
	{
	public:
		JandyRawDataGenerator();

	public:
		virtual boost::asio::awaitable<void> GenerateRawDataFromMessage(const Types::JandyMessageTypePtr& msg) final;
	};

}
// namespace AqualinkAutomate::Generators
