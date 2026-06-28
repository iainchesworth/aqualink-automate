# Security policy

*For anyone deploying or evaluating Aqualink Automate. This page covers which versions get security fixes, how to report a vulnerability privately, and the deployment defaults you should know before exposing the app on a network.*

## Supported versions

Only pre-release (beta) builds exist so far — currently `v0.2.0-beta.2`. There is no stable (non-beta) release line yet, so security fixes are **not** back-ported to earlier betas; they land in development. Run a recent build from the `develop` or `main` branch.

A supported-versions table will be added here at the first stable (non-beta) release.

| Version | Supported |
| ------- | --------- |
| (none yet) | A table will appear here at the first stable (non-beta) release. |

## Reporting a vulnerability

Report security vulnerabilities **privately**. Do **not** open a public GitHub issue, pull request, or discussion for a security problem — a public report exposes the flaw to everyone before a fix exists.

Use one of these private channels instead:

1. **GitHub private vulnerability reporting (preferred).** Open the repository's **Security** tab and choose **Report a vulnerability**. This creates a private GitHub Security Advisory visible only to you and the maintainers. If you do not see the option, private reporting may not be enabled yet — use the contact method below.
2. **Direct contact.** Email the maintainer listed on the GitHub profile of the repository owner. Keep the report private; do not CC public lists.

**Important:** If you are unsure whether something is a security issue, treat it as one and use a private channel. You can always move a non-issue to a public [non-security issue](CONTRIBUTING.md) later, but you cannot un-publish a leaked vulnerability.

Expect an acknowledgement and triage through the private advisory. Fixes are incorporated into development, and the advisory is published once a fix is available.

## What to include in a report

A good report lets the maintainers reproduce and assess the issue quickly. Include:

- **Affected component** — for example the HTTP API, the WebSocket layer, MQTT publishing, or the RS-485 serial protocol parser.
- **Version or commit** — the branch and commit SHA you are running (`git rev-parse HEAD`).
- **How it was deployed** — the relevant CLI flags or config-file keys, especially the bind address (`--address`), whether API auth (`--api-auth-token`) is enabled, and whether MQTT TLS (`--mqtt-tls`) is in use.
- **Reproduction steps** — a minimal sequence, request, or input that triggers the problem.
- **Impact** — what an attacker can read, change, or disrupt.
- **Suggested remediation** — if you have one.

Do not include live credentials or private certificate keys in the report. Redact tokens and passwords.

## Deployment security defaults

Aqualink Automate ships with conservative defaults, but you should understand them before exposing the app beyond your own machine. Config-file keys are the option long name without the leading dashes (for example `--api-auth-token` becomes `api-auth-token`). The full option reference lives in [Configuration reference](configuration.md); the request-auth model is described in detail in [Usage and API](usage-and-api.md).

| Default | Behavior | How to harden |
| ------- | -------- | ------------- |
| Bind address `127.0.0.1` | The web server listens on localhost only; it is not reachable from the network. | Only change this when you intend remote access. Set `--address 0.0.0.0` (or a specific interface) to expose it, and pair that with auth and TLS. |
| API auth **off** | The HTTP API, metrics, and WebSocket are open — no token is required. | Set `--api-auth-token <token>` to require a bearer token (see below). |
| MQTT credentials in **cleartext** | Without `--mqtt-tls`, the MQTT username and password are sent unencrypted; the app logs a warning at startup. | Enable `--mqtt-tls`. The startup warning reads: `MQTT credentials configured without TLS - password will be sent in cleartext; enable --mqtt-tls`. |
| `--mqtt-tls-skip-verify` available | When set, TLS certificate verification is skipped for the MQTT connection. | **Security:** Leave this off in production. It defeats the protection TLS provides against a man-in-the-middle. Use it only for local testing against a self-signed broker. |
| HTTPS certificate **generated per install** | If no certificate is configured, the app generates a unique self-signed cert + private key on first boot (the key is written `0600` and its SHA-256 fingerprint logged). No private key is shipped in the package or committed to the repo. | Supply your own trusted certificate with `--cert` / `--cert-key` (both required together), ideally from a CA your clients trust, at a persistent writable path. |

**Security:** If you set `--address 0.0.0.0` to allow remote access, enable `--api-auth-token` and serve over HTTPS. Binding to all interfaces with auth off exposes pool control to anyone on the network. When the app binds a non-loopback address with no `--api-auth-token`, it logs a prominent startup warning; pass `--insecure-no-auth` to acknowledge an intentionally open deployment (e.g. behind a trusted reverse proxy) and downgrade that warning. Prefer delivering secrets (`--api-auth-token`, `--mqtt-password`) via the config file (readable only by the service user) or the environment rather than the command line, where they are visible in the process table (`/proc/<pid>/cmdline`, `ps`) and shell history.

### How auth enforcement behaves

When you set `--api-auth-token`, the server requires `Authorization: Bearer <token>` on every API request, the `/metrics` endpoint, and the WebSocket upgrade. A missing or mismatched token is rejected with HTTP 401; a request that fails the Origin allow-list is rejected with HTTP 403.

```http
GET /api/equipment HTTP/1.1
Host: 127.0.0.1
Authorization: Bearer your-secret-token
```

Key points:

- **Static assets stay open** so the login screen can load before the user supplies a token. Only `/api`, `/metrics`, and the WebSocket upgrade are gated.
- **WebSocket upgrades** cannot carry an `Authorization` header from a browser, so the token may instead be supplied as a `bearer.<token>` entry in the `Sec-WebSocket-Protocol` header.
- **Origin allow-list and CSRF header** are built into the routing layer and can be enabled:
  - `--api-allowed-origin <origin>` (repeatable) — when set, an API request or WebSocket upgrade whose `Origin` header is not on the list is rejected with HTTP 403. This blocks cross-site WebSocket hijacking and cross-origin reads. Leave unset to disable the check.
  - `--api-require-csrf-header` — when set, state-changing requests (`POST`/`PUT`/`PATCH`/`DELETE`) must carry an `X-Requested-With` header, mitigating cross-site request forgery from a browser. Defaults to off so existing programmatic clients are unaffected.
- **Token strength / brute force.** The token is compared in constant time, but there is no per-IP lockout, so a short or low-entropy token can be guessed online. Use a long random token (32+ characters); the app warns at startup if the configured token is shorter than 16 characters.

To require TLS for the API itself and pick certificates, see the `--cert`, `--cert-key`, and related flags in the [Configuration reference](configuration.md).

## Policy adoption

This security policy is adapted from common open-source practice. Aqualink Automate is licensed under the GNU General Public License v3 (see [LICENSE.txt](https://github.com/iainchesworth/aqualink-automate/blob/main/LICENSE.txt)).

Suggestions to improve this policy are welcome — raise them as a **non-security** issue or pull request through [CONTRIBUTING.md](CONTRIBUTING.md). Keep actual vulnerability reports on the private channels described above.
