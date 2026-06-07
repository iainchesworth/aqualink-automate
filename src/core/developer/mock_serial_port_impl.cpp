#include <filesystem>

#include "developer/mock_serial_port_impl.h"

namespace AqualinkAutomate::Developer
{
	MockSerialPortImpl::MockSerialPortImpl() :
		m_RandomDevice{},
		m_Distribution(32, 127),
		m_ProfilingDomain(std::move(Factory::ProfilerFactory::Instance().Get()->CreateDomain("MockSerialPortImpl")))
	{
	}

	MockSerialPortImpl::MockSerialPortImpl(const std::string& device_name)
		: MockSerialPortImpl()
	{
		open(device_name);
	}

	MockSerialPortImpl::MockSerialPortImpl(const std::string& device_name, boost::system::error_code& ec)
		: MockSerialPortImpl()
	{
		open(device_name, ec);
	}

	MockSerialPortImpl::~MockSerialPortImpl()
	{
		boost::system::error_code ec;
		close(ec);
	}

	void MockSerialPortImpl::open(const std::string& device_name)
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

	void MockSerialPortImpl::open(const std::string& device_name, boost::system::error_code& ec)
	{
		// We're not actually opening a real serial port, so this function
		// doesn't do anything except set the "is_open" flag.

		ec = {};

		if (is_open())
		{
			ec = boost::asio::error::already_open;
		}
		else if (std::filesystem::exists(device_name) && (m_File.open(device_name), m_File.is_open()))
		{
			m_MockData = false;
			m_IsOpen = true;
			m_DeviceName = device_name;
		}
		else
		{
			m_MockData = true;
			m_IsOpen = true;
			m_DeviceName = device_name;
		}
	}

	bool MockSerialPortImpl::is_open() const
	{
		return m_IsOpen;
	}

	void MockSerialPortImpl::cancel()
	{
		boost::system::error_code ec;
		cancel(ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
	}

	void MockSerialPortImpl::cancel(boost::system::error_code& ec)
	{
		ec = {};

		if (!m_IsOpen)
		{
			ec = boost::asio::error::bad_descriptor;
		}
		else
		{
			m_Cancelled = true;
		}
	}

	void MockSerialPortImpl::close()
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

	void MockSerialPortImpl::close(boost::system::error_code& ec)
	{
		// Again, we're not actually closing a real serial port, so this
		// function just sets the "is_open" flag to false.

		ec = {};

		m_DeviceName.clear();
		m_IsOpen = false;
		m_MockData = true;

		try
		{
			if (m_File && m_File.is_open()) m_File.close();
		}
		catch (const std::exception&)
		{
			ec = boost::asio::error::operation_aborted;
		}
	}

	void MockSerialPortImpl::set_baud_rate(uint32_t rate, boost::system::error_code& ec)
	{
		ec = {};
		m_Options.baud_rate = rate;
	}

	void MockSerialPortImpl::set_character_size(uint8_t bits, boost::system::error_code& ec)
	{
		ec = {};
		m_Options.character_size = bits;
	}

	void MockSerialPortImpl::set_flow_control(Serial::FlowControl fc, boost::system::error_code& ec)
	{
		ec = {};
		m_Options.flow_control = fc;
	}

	void MockSerialPortImpl::set_parity(Serial::Parity p, boost::system::error_code& ec)
	{
		ec = {};
		m_Options.parity = p;
	}

	void MockSerialPortImpl::set_stop_bits(Serial::StopBits sb, boost::system::error_code& ec)
	{
		ec = {};
		m_Options.stop_bits = sb;
	}

	void MockSerialPortImpl::set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec)
	{
		ec = {};
	}

	std::size_t MockSerialPortImpl::read_some(const boost::asio::mutable_buffer& buffer)
	{
		boost::system::error_code ec;
		auto bytes = read_some(buffer, ec);
		if (ec)
		{
			throw boost::system::system_error(ec, "read_some");
		}
		return bytes;
	}

	std::size_t MockSerialPortImpl::read_some(const boost::asio::mutable_buffer& buffer, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MockSerialPortImpl::read_some", std::source_location::current());

		ec = boost::system::error_code{};
		auto bytes_transferred = std::size_t{ 0 };

		if (!m_IsOpen)
		{
			ec = boost::asio::error::bad_descriptor;
		}
		else if (!m_MockData)
		{
			if (auto result = HandleFileRead(buffer); result)
			{
				bytes_transferred = *result;
			}
			else
			{
				ec = result.error();
			}
		}
		else
		{
			if (auto result = HandleMockRead(buffer); result)
			{
				bytes_transferred = *result;
			}
			else
			{
				ec = result.error();
			}
		}

		return bytes_transferred;
	}

	std::size_t MockSerialPortImpl::write_some(const boost::asio::const_buffer& buffer)
	{
		boost::system::error_code ec;
		auto bytes = write_some(buffer, ec);
		if (ec)
		{
			throw boost::system::system_error(ec, "write_some");
		}
		return bytes;
	}

	std::size_t MockSerialPortImpl::write_some(const boost::asio::const_buffer& buffer, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MockSerialPortImpl::write_some", std::source_location::current());

		ec = boost::system::error_code{};
		auto bytes_transferred = std::size_t{ 0 };

		if (!m_IsOpen)
		{
			ec = boost::asio::error::bad_descriptor;
		}
		else if (!m_MockData)
		{
			if (auto result = HandleFileWrite(buffer); result)
			{
				bytes_transferred = *result;
			}
			else
			{
				ec = result.error();
			}
		}
		else
		{
			if (auto result = HandleMockWrite(buffer); result)
			{
				bytes_transferred = *result;
			}
			else
			{
				ec = result.error();
			}
		}

		return bytes_transferred;
	}

	std::expected<std::size_t, boost::system::error_code> MockSerialPortImpl::HandleMockRead(const boost::asio::mutable_buffer& buffer)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MockSerialPortImpl::HandleMockRead", std::source_location::current());

		const auto length_to_copy = std::min<std::size_t>(buffer.size(), 16);

		auto* buffer_begin = static_cast<uint8_t*>(buffer.data());
		auto* buffer_end = buffer_begin + length_to_copy;

		std::generate(buffer_begin, buffer_end, [&]() -> uint8_t
			{
				return static_cast<uint8_t>(m_Distribution(m_RandomDevice));
			}
		);

		return length_to_copy;
	}

	std::expected<std::size_t, boost::system::error_code> MockSerialPortImpl::HandleMockWrite(const boost::asio::const_buffer& buffer)
	{
		std::size_t bytes_transferred = 0;

		auto convert_stop_bits_to_number = [](Serial::StopBits stop_bits)
			{
				switch (stop_bits)
				{
				case Serial::StopBits::One: return 1.0;
				case Serial::StopBits::OnePointFive: return 1.5;
				case Serial::StopBits::Two: return 2.0;
				default: LogWarning(Channel::Serial, "Attempted to convert an invalid stop bit value; assuming one (1)"); return 1.0;
				}
			};

		auto convert_parity_bits_to_number = [](Serial::Parity the_parity)
			{
				switch (the_parity)
				{
				case Serial::Parity::None:
					return 0;

				case Serial::Parity::Odd:
					[[fallthrough]];
				case Serial::Parity::Even:
					return 1;

				default:
					LogWarning(Channel::Serial, "Attempted to convert an invalid parity value; assuming none (0)");
					return 0;
				}
			};

		// The total bits transmitted per character is: start_bit + data_bits + optional_parity_bit + stop_bits
		const double bits_per_sent_byte = 1.0 + static_cast<double>(m_Options.character_size) + static_cast<double>(convert_parity_bits_to_number(m_Options.parity)) + convert_stop_bits_to_number(m_Options.stop_bits);
		const double number_of_bits_to_send = bits_per_sent_byte * static_cast<double>(buffer.size());
		const auto bits_per_msec = static_cast<double>(m_Options.baud_rate) / 1000.0;
		const auto send_duration = std::chrono::milliseconds(static_cast<long long>(number_of_bits_to_send / bits_per_msec));

		std::this_thread::sleep_for(send_duration);
		bytes_transferred = buffer.size();

		return bytes_transferred;
	}

	std::expected<std::size_t, boost::system::error_code> MockSerialPortImpl::HandleFileRead(const boost::asio::mutable_buffer& buffer)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MockSerialPortImpl::HandleFileRead", std::source_location::current());

		enum FileReadErrors : std::size_t
		{
			ErrorFileReachedEOF,
			ErrorDuringRead,
			NoDataWasRead,
			ReadSuccessfully
		};

		auto ec = boost::system::error_code{};

		auto read_single_value_from_file = [](auto& source_stream, uint8_t& output_buffer) -> FileReadErrors
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MockSerialPortImpl::HandleFileRead -> read_single_value", std::source_location::current());

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
						// Expected format is 0x## (4 chars); skip malformed entries.
						LogWarning(Channel::Serial, std::format("Read data from the source file however it was not in the expected format (0x##); sequence -> {}", line));
					}
					else
					{
						uint8_t converted_value = 0;

						auto [p, conv_ec] = std::from_chars(line.data() + 2, line.data() + 4, converted_value, 16);
						if (std::errc() == conv_ec)
						{
							output_buffer = converted_value;
							return_value = ReadSuccessfully;
						}
						else if (conv_ec == std::errc::result_out_of_range)
						{
							LogTrace(Channel::Serial, std::format("Could not convert data read from file (out-of-range); sequence -> {}", line));
						}
						else if (conv_ec == std::errc::invalid_argument)
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
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MockSerialPortImpl::HandleFileRead -> read_block", std::source_location::current());

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
						break;

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
		auto length_read = read_from_file(m_File, static_cast<uint8_t*>(buffer.data()), length_to_copy, ec);

		if (ec)
		{
			return std::unexpected(ec);
		}

		return length_read;
	}

	std::expected<std::size_t, boost::system::error_code> MockSerialPortImpl::HandleFileWrite(const boost::asio::const_buffer& buffer)
	{
		return 0;
	}

}
// namespace AqualinkAutomate::Developer
