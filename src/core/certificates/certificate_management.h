#pragma once

#include <filesystem>
#include <optional>

#include <boost/asio/ssl/context.hpp>

#include "options/options_web_options.h"

namespace AqualinkAutomate::Certificates
{

	void LoadSslCertificates(const AqualinkAutomate::Options::Web::WebSettings& cfg, boost::asio::ssl::context &ctx);

	// Generate a fresh, self-signed TLS certificate + 2048-bit RSA private key at
	// the given paths (CN=localhost, SAN DNS:localhost + IP:127.0.0.1, ~10y
	// validity, random serial). The private key is written with owner-only (0600)
	// permissions and the certificate's SHA-256 fingerprint is logged. Returns
	// true on success. Used to give every install a UNIQUE key instead of the old
	// shared, repository-committed one.
	bool GenerateSelfSignedCertificate(const std::filesystem::path& certificate_path, const std::filesystem::path& private_key_path);

	// Ensure usable TLS material exists for the configured cert/key. If both files
	// already exist they are returned unchanged. Otherwise -- but ONLY when the
	// configured paths are the built-in defaults (so an operator's explicit
	// --cert/--cert-key that is missing still fails loudly) -- a per-install
	// self-signed pair is generated, preferring the configured directory and
	// falling back to a writable runtime directory when that is read-only. Returns
	// the cert+key paths actually in use, or std::nullopt if none could be produced.
	std::optional<AqualinkAutomate::Options::Web::SslCertificate> EnsureSelfSignedMaterial(const AqualinkAutomate::Options::Web::SslCertificate& configured);

}
// namespace AqualinkAutomate::Certificates
