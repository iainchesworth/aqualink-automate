#include <format>

#include "certificates/certificate_management.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Certificates
{

    void LoadWebCertificates(const AqualinkAutomate::Options::Web::Settings& cfg, boost::asio::ssl::context& ctx)
    {
        LogTrace(Channel::Certificates, "Certificates::LoadCertificates");

        boost::system::error_code ec;

        if (!cfg.http_server_is_insecure)
        {
            LogDebug(Channel::Certificates, "Certificates::LoadCertificates - insecure option has been used, not loading certificates");
        }
        else if (!cfg.cert_file.IsValid())
        {
            LogDebug(Channel::Certificates, "Certificates::LoadCertificates - invalid server certificate path provided; ignoring all certificates");
        }
        else if (!cfg.cert_key_file.IsValid())
        {
            LogDebug(Channel::Certificates, "Certificates::LoadCertificates - invalid server certificate key path provided; ignoring all certificates");
        }
        else if (ctx.use_certificate(cfg.cert_file.Data(), boost::asio::ssl::context::pem, ec); ec)
        {
            LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: Cannot Load Certificate File - Error: {}", ec.message()));
        }
        else if (ctx.use_private_key(cfg.cert_key_file.Data(), boost::asio::ssl::context::pem); ec)
        {
            LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: Cannot Load Certificate Key File - Error: {}", ec.message()));
        }
        else
        {
            if (!cfg.ca_chain_cert_file.IsValid())
            {
                LogDebug(Channel::Certificates, "Certificates::LoadCertificates - no ca chain certificate path provided; ignoring ca chain certificate");
            }
            else if (ctx.use_certificate_chain(cfg.ca_chain_cert_file.Data()); ec)
            {
                LogWarning(Channel::Certificates, std::format("Certificates::LoadCertificates Failure: Cannot Load CA Chain Certificate File - Error: {}", ec.message()));
            }
            else
            {
            }

            LogDebug(Channel::Certificates, "Certificates::LoadCertificates - webserver certificates loaded successfully");
        }
    }

}
// namespace AqualinkAutomate::Certificates
