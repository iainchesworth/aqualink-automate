#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <expected>
#include <queue>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

#include "developer/mock_serial_port_impl.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Developer;

namespace AqualinkAutomate::Test
{

	class TestSerialPortImpl : public Developer::MockSerialPortImpl
	{
	public:
		struct QueuedReadDataType
		{
			std::vector<uint8_t> data;
			boost::system::error_code error;
		};

		struct QueuedWriteResponseType
		{
			std::size_t bytes_to_return;
			boost::system::error_code error;
		};

		TestSerialPortImpl() :
			Developer::MockSerialPortImpl(),
			m_TestMode(false)
		{
			// Open with a fake device name to make it operational
			boost::system::error_code ec;
			open("test_device", ec);
		}

		TestSerialPortImpl(const TestSerialPortImpl&) = delete;
		TestSerialPortImpl& operator=(const TestSerialPortImpl&) = delete;
		TestSerialPortImpl(TestSerialPortImpl&& other) noexcept = delete;
		TestSerialPortImpl& operator=(TestSerialPortImpl&& other) noexcept = delete;

		void EnableTestMode(bool enable = true)
		{
			m_TestMode = enable;
		}

		void QueueReadData(const std::vector<uint8_t>& data, boost::system::error_code ec = {})
		{
			QueuedReadDataType queued;
			queued.data = data;
			queued.error = ec;
			m_ReadQueue.push(queued);
		}

		void QueueReadError(boost::system::error_code ec)
		{
			QueuedReadDataType queued;
			queued.error = ec;
			m_ReadQueue.push(queued);
		}

		void QueueReadDataBatch(const std::vector<std::vector<uint8_t>>& batch)
		{
			for (const auto& data : batch)
			{
				QueueReadData(data);
			}
		}

		void QueueWriteResponse(std::size_t bytes_written, boost::system::error_code ec = {})
		{
			QueuedWriteResponseType queued;
			queued.bytes_to_return = bytes_written;
			queued.error = ec;
			m_WriteQueue.push(queued);
		}

		void QueueWriteError(boost::system::error_code ec)
		{
			QueuedWriteResponseType queued;
			queued.bytes_to_return = 0;
			queued.error = ec;
			m_WriteQueue.push(queued);
		}

		// Statistics getters
		std::size_t GetReadCallCount() const { return m_ReadCallCount; }
		std::size_t GetWriteCallCount() const { return m_WriteCallCount; }
		std::size_t GetTotalBytesRead() const { return m_TotalBytesRead; }
		std::size_t GetTotalBytesWritten() const { return m_TotalBytesWritten; }
		std::size_t GetReadErrorCount() const { return m_ReadErrorCount; }
		std::size_t GetWriteErrorCount() const { return m_WriteErrorCount; }
		const std::vector<uint8_t>& GetWrittenData() const { return m_WrittenData; }

		// Test control
		void ClearWrittenData() { m_WrittenData.clear(); }
		void ClearQueues()
		{
			while (!m_ReadQueue.empty()) m_ReadQueue.pop();
			while (!m_WriteQueue.empty()) m_WriteQueue.pop();
		}
		void ResetStatistics()
		{
			m_ReadCallCount = 0;
			m_WriteCallCount = 0;
			m_TotalBytesRead = 0;
			m_TotalBytesWritten = 0;
			m_ReadErrorCount = 0;
			m_WriteErrorCount = 0;
		}
		void Reset()
		{
			ClearQueues();
			ClearWrittenData();
			ResetStatistics();
		}

		bool IsReadQueueEmpty() const { return m_ReadQueue.empty(); }
		bool IsWriteQueueEmpty() const { return m_WriteQueue.empty(); }

		std::size_t read_some(const boost::asio::mutable_buffer& buffer, boost::system::error_code& ec) override
		{
			m_ReadCallCount++;

			if (m_TestMode && !m_ReadQueue.empty())
			{
				auto queued = m_ReadQueue.front();
				m_ReadQueue.pop();

				if (queued.error)
				{
					m_ReadErrorCount++;
					ec = queued.error;
					return 0;
				}

				// Copy queued data to buffer
				std::size_t bytes_to_copy = std::min(queued.data.size(), buffer.size());
				std::memcpy(buffer.data(), queued.data.data(), bytes_to_copy);

				m_TotalBytesRead += bytes_to_copy;
				ec = {};
				return bytes_to_copy;
			}

			// Fall back to base class behavior (random data or file)
			auto result = Developer::MockSerialPortImpl::read_some(buffer, ec);
			if (!ec)
			{
				m_TotalBytesRead += result;
			}
			else
			{
				m_ReadErrorCount++;
			}

			return result;
		}

		std::size_t write_some(const boost::asio::const_buffer& buffer, boost::system::error_code& ec) override
		{
			m_WriteCallCount++;

			// Always capture what was written for inspection
			auto buffer_ptr = static_cast<const uint8_t*>(buffer.data());
			m_WrittenData.insert(m_WrittenData.end(), buffer_ptr, buffer_ptr + buffer.size());

			if (m_TestMode && !m_WriteQueue.empty())
			{
				auto queued = m_WriteQueue.front();
				m_WriteQueue.pop();

				if (queued.error)
				{
					m_WriteErrorCount++;
					ec = queued.error;
					return 0;
				}

				std::size_t bytes_written = std::min(queued.bytes_to_return, buffer.size());
				m_TotalBytesWritten += bytes_written;
				ec = {};
				return bytes_written;
			}

			// Fall back to base class behavior
			auto result = Developer::MockSerialPortImpl::write_some(buffer, ec);

			if (!ec)
			{
				m_TotalBytesWritten += result;
			}
			else
			{
				m_WriteErrorCount++;
			}

			return result;
		}

	private:
		bool m_TestMode;
		std::queue<QueuedReadDataType> m_ReadQueue;
		std::queue<QueuedWriteResponseType> m_WriteQueue;
		std::vector<uint8_t> m_WrittenData;

		std::size_t m_ReadCallCount = 0;
		std::size_t m_WriteCallCount = 0;
		std::size_t m_TotalBytesRead = 0;
		std::size_t m_TotalBytesWritten = 0;
		std::size_t m_ReadErrorCount = 0;
		std::size_t m_WriteErrorCount = 0;

	};

}
// namespace AqualinkAutomate::Test
