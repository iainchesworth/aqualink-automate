#pragma once

#include <filesystem>
#include <optional>
#include <vector>

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

	namespace Detail
	{

		// Walk 'candidate_dirs' in order and, in the FIRST directory that can be
		// created and written to, either reuse an existing cert.pem/key.pem pair or
		// generate a fresh self-signed pair there. Returns the cert+key paths in use,
		// or std::nullopt if no candidate directory was usable.
		//
		// This is the read-only-install fallback that backs EnsureSelfSignedMaterial;
		// it is exposed here (rather than kept in the .cpp's unnamed namespace) so the
		// security-critical generate/reuse/skip-unwritable path can be unit-tested
		// with controlled directories, without depending on the built-in default cert
		// paths being writable.
		std::optional<AqualinkAutomate::Options::Web::SslCertificate> GenerateOrReuseInDirectories(const std::vector<std::filesystem::path>& candidate_dirs);

	}
	// namespace Detail

}
// namespace AqualinkAutomate::Certificates
