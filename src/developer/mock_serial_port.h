#pragma once

#include <algorithm>
#include <chrono>
#include <coroutine>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <random>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/recycling_allocator.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/system/system_error.hpp>

#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Developer
{
	struct mock_serial_port_options
	{
		boost::asio::serial_port::baud_rate baud_rate{ 9600 };
		boost::asio::serial_port::parity parity{ boost::asio::serial_port_base::parity::none };
		boost::asio::serial_port::character_size character_size{ 8 };
		boost::asio::serial_port::stop_bits stop_bits{ boost::asio::serial_port_base::stop_bits::one };
		boost::asio::serial_port::flow_control flow_control{ boost::asio::serial_port::flow_control::none };
	};

	class mock_serial_port
	{
	public:
		explicit mock_serial_port(boost::asio::io_context& io_context);
		mock_serial_port(boost::asio::io_context& io_context, const std::string& device_name);
		mock_serial_port(boost::asio::io_context& io_context, const std::string& device_name, boost::system::error_code& ec);
		~mock_serial_port();

		mock_serial_port(const mock_serial_port&) = default;
		mock_serial_port(mock_serial_port&&) noexcept = default;
		mock_serial_port& operator=(const mock_serial_port&) = default;
		mock_serial_port& operator=(mock_serial_port&&) noexcept = default;

		void open(const std::string& device_name);
		void open(const std::string& device_name, boost::system::error_code& ec);

		bool is_open() const;

		void close();
		void close(boost::system::error_code& ec);

		void cancel();
		void cancel(boost::system::error_code& ec);

		boost::asio::executor get_executor();

		void set_option(const boost::asio::serial_port_base::baud_rate& option, boost::system::error_code& ec);
		void set_option(const boost::asio::serial_port_base::character_size& option, boost::system::error_code& ec);
		void set_option(const boost::asio::serial_port_base::flow_control& option, boost::system::error_code& ec);
		void set_option(const boost::asio::serial_port_base::parity& option, boost::system::error_code& ec);
		void set_option(const boost::asio::serial_port_base::stop_bits& option, boost::system::error_code& ec);

		template <typename MutableBufferSequence, boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)> ReadToken>
		BOOST_ASIO_INITFN_RESULT_TYPE(ReadToken, void(boost::system::error_code, std::size_t)) async_read_some(const MutableBufferSequence& buffer, ReadToken&& token)
		{
			static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("mock_serial_port -> async_read_some", std::source_location::current()));

			auto init = [&](boost::asio::completion_handler_for<void(boost::system::error_code, std::size_t)> auto handler, const MutableBufferSequence& buffer_)
			{
				auto work = boost::asio::make_work_guard(handler);
				auto ec = boost::system::error_code{};
				auto bytes_transferred = 0;

				if (!m_IsOpen)
				{
					ec = boost::asio::error::bad_descriptor;
				}
				else if (!m_MockData)
				{
					bytes_transferred = HandleFileRead(buffer_, ec);
				}
				else
				{
					bytes_transferred = HandleMockRead(buffer_, ec);
				}

				auto alloc = boost::asio::get_associated_allocator(handler, boost::asio::recycling_allocator<void>());

				boost::asio::post(work.get_executor(), boost::asio::bind_allocator(alloc, [=, handler_ = std::move(handler), ec_ = boost::system::error_code(ec), bytes_transferred_ = std::size_t(bytes_transferred)]() mutable
					{
						std::move(handler_)(ec_, bytes_transferred_);
					})
				);
			};

			return boost::asio::async_initiate<ReadToken, void(boost::system::error_code, std::size_t)>(init, token, buffer);
		}

		template <typename ConstBufferSequence, boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)> WriteToken>
		BOOST_ASIO_INITFN_RESULT_TYPE(WriteToken,void(boost::system::error_code, std::size_t)) async_write_some(const ConstBufferSequence& buffer, WriteToken&& handler)
		{
			static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("mock_serial_port -> async_write_some", std::source_location::current()));

			auto ec = boost::system::error_code{};
			auto bytes_transferred = 0;

			if (!m_IsOpen)
			{
				ec = boost::asio::error::bad_descriptor;
			}
			else
			{
				auto convert_stop_bits_to_number = [](boost::asio::serial_port::stop_bits stop_bits)
				{
					switch (stop_bits)
					{
					case boost::asio::serial_port::stop_bits::one: return 1.0;
					case boost::asio::serial_port::stop_bits::onepointfive: return 1.5;
					case boost::asio::serial_port::stop_bits::two: return 2.0;
					}
				};

				auto convert_parity_bits_to_number = [](boost::asio::serial_port::parity the_parity)
				{
					switch (the_parity)
					{
					case boost::asio::serial_port::parity::none:
						return 0;

					case boost::asio::serial_port::parity::odd:
						[[fallthrough]]
					case boost::asio::serial_port::parity::even:
						return 1;
					}
				};

				// The total bits transmitted per character is: start_bit + data_bits + optional_parity_bit + stop_bits
				const auto bits_per_sent_byte = 1 + m_Options.character_size.value() + convert_parity_bits_to_number(m_Options.parity) + convert_stop_bits_to_number(m_Options.stop_bits);
				const auto number_of_bits_to_send = bits_per_sent_byte * buffer.size();
				const auto baud_rate_in_us = m_Options.baud_rate.value() / std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(1)).count();
				const auto send_duration = std::chrono::duration_cast<std::chrono::microseconds>(number_of_bits_to_send / baud_rate_in_us);

				m_WriteDelayTimer.expires_after(send_duration);
				m_WriteDelayTimer.async_wait([](const boost::system::error_code& error)
					{
						// Nothing happens here because we're merely writing the data onto the pretend wire.
						bytes_transferred = buffer.size();
					}
				);
			};

			// Call the handler...
			handler(ec, bytes_transferred);
		}

	private:
		template <typename MutableBufferSequence>
		std::size_t HandleMockRead(const MutableBufferSequence& buffer, boost::system::error_code& ec)
		{
			static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("mock_serial_port -> HandleMockRead", std::source_location::current()));

			const auto length_to_copy = std::min<std::size_t>(boost::asio::buffer_size(buffer), 16);

			auto buffer_begin = boost::asio::buffers_begin(buffer);
			auto buffer_end = buffer_begin + length_to_copy;

			std::mt19937 gen(m_RandomDevice());
			std::generate(buffer_begin, buffer_end, [&]()
				{
					return m_Distribution(gen);
				}
			);

			return length_to_copy;
		}

	private:
		template <typename MutableBufferSequence>
		std::size_t HandleFileRead(const MutableBufferSequence& buffer, boost::system::error_code& ec)
		{
			static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("mock_serial_port -> HandleFileRead", std::source_location::current()));

			enum FileReadErrors : std::size_t
			{
				ErrorFileReachedEOF,
				ErrorDuringRead,
				NoDataWasRead,
				ReadSuccessfully
			};

			auto read_single_value_from_file = [](auto& source_stream, uint8_t& output_buffer) -> FileReadErrors
			{
				static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("mock_serial_port -> read_single_value_from_file", std::source_location::current()));

				FileReadErrors return_value = NoDataWasRead;

				if (source_stream.eof())
				{
					return_value = ErrorFileReachedEOF;
				}
				else
				{
					std::string line;
									
					if (std::getline(source_stream, line, '|'); source_stream.eof() || source_stream.fail() || source_stream.bad())
					{
						LogTrace(Channel::Serial, "Failed to read data from the source file; either an error occurred or file reached eof.");
						return_value = source_stream.eof() ? ErrorFileReachedEOF : ErrorDuringRead;
					}
					else if (4 != line.size())
					{
						/// TODO Didn't get a value in the format 0x##...
						LogDebug(Channel::Serial, std::format("Read data from the source file however it was not in the expected forrmat; sequence -> {}", line));
					}
					else
					{
						uint8_t converted_value;

						auto [p, ec] = std::from_chars(line.data()+2, line.data()+4, converted_value, 16);
						if (std::errc() == ec)
						{
							output_buffer = converted_value;
							return_value = ReadSuccessfully;
						}
						else if (ec == std::errc::result_out_of_range)
						{
							LogTrace(Channel::Serial, std::format("Could not convert data read from file (out-of-range); sequence -> {}", line));
						}
						else if (ec == std::errc::invalid_argument)
						{
							LogTrace(Channel::Serial, std::format("Could not convert data read from file (invalid argument); sequence -> {}", line));
						}
						else
						{
							LogTrace(Channel::Serial, std::format("Could not convert data read from file (unknown error); sequence -> {}", line));
						}
					}					
				}

				return return_value;
			};

			auto read_from_file = [&](auto& source_stream, uint8_t* output_buffer, std::size_t number_of_elems, boost::system::error_code& ec) -> std::size_t
			{
				static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("mock_serial_port -> read_from_file", std::source_location::current()));

				std::size_t elems_read = 0;
				bool keep_reading = true;

				do
				{
					const FileReadErrors result = read_single_value_from_file(source_stream, output_buffer[elems_read]);
					switch (result)
					{
					case FileReadErrors::ErrorFileReachedEOF:
						ec = boost::asio::error::eof;
						keep_reading = false;

					case FileReadErrors::ErrorDuringRead:
					case FileReadErrors::NoDataWasRead:
						ec = boost::asio::error::operation_aborted;
						keep_reading = false; 
						break;

					case FileReadErrors::ReadSuccessfully:
						ec = make_error_code(boost::system::errc::success);
						elems_read++; 
						break;
					}

				} while (keep_reading && (number_of_elems > elems_read));
				
				return elems_read;
			};

			const auto length_to_copy = boost::asio::buffer_size(buffer);
			auto length_read = read_from_file(m_File, static_cast<uint8_t *>(buffer.data()), length_to_copy, ec);

			return length_read;
		}

	private:
		mock_serial_port_options m_Options;
		
	private:
		boost::asio::io_context& m_IOContext;
		boost::asio::steady_timer m_WriteDelayTimer;
		std::random_device m_RandomDevice;
		std::uniform_int_distribution<> m_Distribution;
		std::string m_DeviceName;
		bool m_MockData{ true };
		bool m_IsOpen{ false };

	private:
		boost::iostreams::stream<boost::iostreams::file_source> m_File;
	};

}
// namespace AqualinkAutomate::Developer
