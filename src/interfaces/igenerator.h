#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <vector>

#include <boost/asio/awaitable.hpp>

#include "logging/logging.h"

namespace AqualinkAutomate::Interfaces
{
	template<typename SERIAL_DATA_TYPE, typename EXPECTED_MESSAGE_TYPE>
	class IGenerator
	{
	public:
		IGenerator() : 
			m_SerialData()
		{
		}

		virtual ~IGenerator()
		{
		}

	public:
		void InjectRawSerialData(const std::span<const SERIAL_DATA_TYPE>& read_buffer)
		{
			// Step 1 -> push the new bytes into the internal buffer.
			LogTrace(Logging::Channel::Messages, std::format("Existing serial buffer contains {} elements, adding {} new elements.", m_SerialData.size(), read_buffer.size()));
			m_SerialData.insert(m_SerialData.end(), read_buffer.begin(), read_buffer.end());
		}

	protected:
		std::vector<SERIAL_DATA_TYPE> m_SerialData;
	};

	template<typename SERIAL_DATA_TYPE, typename EXPECTED_MESSAGE_TYPE>
	class IGenerator_RawDataToMessage : public IGenerator<SERIAL_DATA_TYPE, EXPECTED_MESSAGE_TYPE>
	{
	public:
		virtual boost::asio::awaitable<EXPECTED_MESSAGE_TYPE> GenerateMessageFromRawData() = 0;
	};

	template<typename SERIAL_DATA_TYPE, typename MESSAGE_TYPE>
	class IGenerator_MessageToRawData
	{
	public:
		virtual boost::asio::awaitable<void> GenerateRawDataFromMessage(const MESSAGE_TYPE& msg) = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
