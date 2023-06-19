#include <fstream>
#include <iostream>

#include "certificates/certificate_pemformat.h"

namespace AqualinkAutomate::Certificates
{

	Certificate_PemFormat::Certificate_PemFormat() :
		ICertificate()
	{
	}

	Certificate_PemFormat::Certificate_PemFormat(const Certificate_PemFormat& other) :
		ICertificate(other),
		m_Valid(other.m_Valid)
	{
	}

	Certificate_PemFormat::Certificate_PemFormat(const std::string& file_path) :
		ICertificate()
	{
		m_Valid = ReadIntoBuffer(file_path, m_Data);
	}

	Certificate_PemFormat& Certificate_PemFormat::operator=(const Certificate_PemFormat& other)
	{
		if (this != &other)
		{
			ICertificate::operator=(other);
			m_Valid = other.m_Valid;
		}

		return *this;
	}

	Certificate_PemFormat& Certificate_PemFormat::operator=(const std::string& file_path)
	{
		m_Valid = ReadIntoBuffer(file_path, m_Data);
		return *this;
	}

	bool Certificate_PemFormat::IsFile() const
	{
		return m_Valid; // If this is not valid then it is not a file.
	}

	bool Certificate_PemFormat::IsValid() const
	{
		return m_Valid;
	}

	bool Certificate_PemFormat::ReadIntoBuffer(const std::string& file_path, std::string& result)
	{
		std::ifstream file(file_path, std::ios::in | std::ios::binary | std::ios::ate);

		if (file)
		{
			const std::streamsize file_size = file.tellg();

			if (file_size <= 0 || static_cast<std::size_t>(file_size) > MAX_CERTIFICATE_SIZE)
			{
				return false;
			}

			file.seekg(0, std::ios::beg);

			result.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

			return true;
		}
		else
		{
			return false;
		}
	}

}
// namespace AqualinkAutomate::Certificates
