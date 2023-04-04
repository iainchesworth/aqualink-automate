#pragma once

#include <algorithm>
#include <chrono>
#include <coroutine>
#include <memory>
#include <string>
#include <random>
#include <utility>
#include <vector>

#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/recycling_allocator.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/system_error.hpp>

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
		explicit mock_serial_port(boost::asio::io_context& io_context) : 
			m_IOContext(io_context),
			m_WriteDelayTimer(m_IOContext),
			m_RandomDevice{},
			m_Distribution(32, 127)
		{
		}

		mock_serial_port(boost::asio::io_context& io_context, const std::string& device_name)
			: mock_serial_port(io_context)
		{
			open(device_name);
		}

		mock_serial_port(boost::asio::io_context& io_context, const std::string& device_name, boost::system::error_code& ec)
			: mock_serial_port(io_context)
		{
			open(device_name, ec);
		}

		~mock_serial_port()
		{
			close();
		}

		mock_serial_port(const mock_serial_port&) = default;
		mock_serial_port(mock_serial_port&&) noexcept = default;
		mock_serial_port& operator=(const mock_serial_port&) = default;
		mock_serial_port& operator=(mock_serial_port&&) noexcept = default;

		void open(const std::string& device_name)
		{
			// We're not actually opening a real serial port, so this function
			// doesn't do anything except set the "is_open" flag.

			boost::system::error_code ec;
			open(device_name, ec);
			if (ec)
			{
				throw boost::system::system_error(ec);
			}
		}

		void open(const std::string& device_name, boost::system::error_code& ec)
		{
			// We're not actually opening a real serial port, so this function
			// doesn't do anything except set the "is_open" flag.

			if (is_open())
			{
				ec = make_error_code(boost::system::errc::device_or_resource_busy);
			}
			else
			{
				ec = {};

				m_IsOpen = true;
				m_DeviceName = device_name;
			}
		}

		bool is_open() const
		{
			return m_IsOpen;
		}

		void close()
		{
			// Again, we're not actually closing a real serial port, so this
			// function just sets the "is_open" flag to false.

			boost::system::error_code ec;
			close(ec);
			if (ec)
			{
				throw boost::system::system_error(ec);
			}
		}

		void close(boost::system::error_code& ec)
		{
			// Again, we're not actually closing a real serial port, so this
			// function just sets the "is_open" flag to false.

			if (!m_IsOpen)
			{
				ec = make_error_code(boost::system::errc::bad_file_descriptor);
			}
			else
			{
				ec = {};
				m_DeviceName.clear();
				m_IsOpen = false;
			}
		}

		void cancel()
		{
			boost::system::error_code ec;
			cancel(ec);
			if (ec)
			{
				throw boost::system::system_error(ec);
			}
		}

		void cancel(boost::system::error_code& ec)
		{
			ec = {};
			m_WriteDelayTimer.cancel();
		}

		boost::asio::executor get_executor()
		{
			return m_IOContext.get_executor();
		}

		void set_option(const boost::asio::serial_port_base::baud_rate& option, boost::system::error_code& ec)
		{
			m_Options.baud_rate = option;
		}

		void set_option(const boost::asio::serial_port_base::character_size& option, boost::system::error_code& ec)
		{
			m_Options.character_size = option;
		}

		void set_option(const boost::asio::serial_port_base::flow_control& option, boost::system::error_code& ec)
		{
			m_Options.flow_control = option;
		}

		void set_option(const boost::asio::serial_port_base::parity& option, boost::system::error_code& ec)
		{
			m_Options.parity = option;
		}

		void set_option(const boost::asio::serial_port_base::stop_bits& option, boost::system::error_code& ec)
		{
			m_Options.stop_bits = option;
		}

		template <typename MutableBufferSequence, boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)> ReadToken>
		BOOST_ASIO_INITFN_RESULT_TYPE(ReadToken, void(boost::system::error_code, std::size_t)) async_read_some(const MutableBufferSequence& buffer, ReadToken&& token)
		{
			auto init = [&](boost::asio::completion_handler_for<void(boost::system::error_code, std::size_t)> auto handler, const MutableBufferSequence& buffer_)
			{
				auto work = boost::asio::make_work_guard(handler);
				auto ec = boost::system::error_code{};
				auto bytes_transferred = 0;

				if (!m_IsOpen)
				{
					ec = boost::asio::error::bad_descriptor;
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
		mock_serial_port_options m_Options;
		
	private:
		boost::asio::io_context& m_IOContext;
		boost::asio::steady_timer m_WriteDelayTimer;
		std::random_device m_RandomDevice;
		std::uniform_int_distribution<> m_Distribution;
		std::string m_DeviceName;
		bool m_IsOpen{ false };
	};

}
// namespace AqualinkAutomate::Developer
