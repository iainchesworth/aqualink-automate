# TLS material (generated per install — nothing secret is committed here)

This directory intentionally ships **no certificate or private key**.

Earlier releases committed a `cert.pem` / `key.pem` / `dh.pem` here and installed
them verbatim into every package. That meant **every installation served HTTPS with
the same private key**, which is published in this public repository — so an on-path
attacker could trivially decrypt or impersonate the control plane. The shared key has
been removed (note: it remains in git history and must be considered compromised — do
not reuse it anywhere).

Instead, on first boot the application generates a **unique, self-signed certificate
and 2048-bit RSA private key** for this install (CN=`localhost`, SAN `localhost` /
`127.0.0.1` / `::1`). The private key is written with owner-only (`0600`) permissions
and its SHA-256 fingerprint is logged. Generation targets the configured certificate
directory when it is writable, otherwise a per-user runtime directory.

For a stable, production certificate (e.g. one issued by your own CA or trusted by your
clients), supply your own with `--cert <cert.pem>` and `--cert-key <key.pem>` (both are
required together), or place the files where `--cert` / `--cert-key` point. A persistent,
writable path is recommended so the certificate does not change between restarts.
