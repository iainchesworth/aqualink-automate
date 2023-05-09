#pragma once

#include <string>

#include "interfaces/icertificate.h"

namespace AqualinkAutomate::Certificates
{

	class Certificate_PemFormat : public Interfaces::ICertificate
	{
		static constexpr std::size_t MAX_CERTIFICATE_SIZE = 4096;

	public:
		Certificate_PemFormat();
		Certificate_PemFormat(const Certificate_PemFormat& other);
		Certificate_PemFormat(const std::string& file_path);

	public:
		Certificate_PemFormat& operator=(const Certificate_PemFormat& other);
		Certificate_PemFormat& operator=(const std::string& file_path);

	public:
		virtual bool IsFile() const final;
		virtual bool IsValid() const final;

	private:
		static bool ReadIntoBuffer(const std::string& file_path, std::string& result);

	private:
		bool m_Valid{ false };
	};

}
// namespace AqualinkAutomate::Certificates
