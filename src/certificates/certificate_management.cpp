#include <filesystem>
#include <format>

#include <boost/system/error_code.hpp>

#include "certificates/certificate_management.h"
#include "exceptions/exception_certificate_invalidformat.h"
#include "exceptions/exception_certificate_notfound.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Certificates
{  

    void LoadSslCertificates(const AqualinkAutomate::Options::Web::Settings& cfg, boost::asio::ssl::context& ctx)
    {
        if (cfg.http_server_is_insecure)
        {
            LogDebug(Channel::Certificates, "Certificates::LoadCertificates - insecure option has been used, not loading certificates");
        }
        else if (!std::filesystem::exists(cfg.ssl_certificate.certificate))
        {
            LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: cannot find specified certificate file; path was -> {}", cfg.ssl_certificate.certificate.string()));
            throw Exceptions::Certificate_NotFound();
        }
        else if (!std::filesystem::exists(cfg.ssl_certificate.private_key))
        {
            LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: cannot find specified private key; path was -> {}", cfg.ssl_certificate.private_key.string()));
            throw Exceptions::Certificate_NotFound();
        }
        else
        {
            boost::system::error_code ec;

            if (ctx.use_certificate_file(cfg.ssl_certificate.certificate.string(), boost::asio::ssl::context::pem, ec); ec)
            {
                LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: Cannot Load Certificate File - Error: {}", ec.message()));
                throw Exceptions::Certificate_InvalidFormat();
            }
            else if (ctx.use_private_key_file(cfg.ssl_certificate.private_key.string(), boost::asio::ssl::context::pem, ec); ec)
            {
                LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: Cannot Load Certificate Key File - Error: {}", ec.message()));
                throw Exceptions::Certificate_InvalidFormat();
            }
            else
            {
                if (!cfg.ca_chain_certificate.has_value())
                {
                    LogTrace(Channel::Certificates, "Certificates::LoadCertificates - CA chain certificate path was not provided; ignoring");
                }
                else if (!std::filesystem::exists(cfg.ca_chain_certificate.value()))
                {
                    LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: cannot find specified ca chain certificate file; path was -> {}", cfg.ca_chain_certificate.value().string()));
                    throw Exceptions::Certificate_NotFound();
                }
                else if (ctx.use_certificate_chain_file(cfg.ca_chain_certificate.value().string(), ec); ec)
                {
                    LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: Cannot Load CA Chain Certificate File - Error: {}", ec.message()));
                    throw Exceptions::Certificate_InvalidFormat();
                }
                else
                {
                }

                LogDebug(Channel::Certificates, "Certificates::LoadCertificates - webserver certificates loaded successfully");
            }
        }
    }

}
// namespace AqualinkAutomate::Certificates
