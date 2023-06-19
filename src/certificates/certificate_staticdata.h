#pragma once

#include <string>

#include "interfaces/icertificate.h"

namespace AqualinkAutomate::Certificates
{

	class Certificate_StaticData : public Interfaces::ICertificate
	{
		static const std::string STATIC_CERTIFICATE_DATA;

	public:
		Certificate_StaticData(const std::string& certificate_data);

	public:
		virtual bool IsFile() const final;
		virtual bool IsValid() const final;

	private:
		bool m_Valid{ false };
	};	

}
// namespace AqualinkAutomate::Certificates
