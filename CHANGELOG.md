# Changelog

*For end users tracking what has shipped. How releases and version numbers are cut lives in [docs/releasing.md](docs/releasing.md); the project overview is in [README.md](README.md).*

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

See [docs/releasing.md](docs/releasing.md) for how releases and version numbers are cut.

## [0.5.0-beta.1] - 2026-06-19

### Added

- **Selectable MQTT protocol version (3.1.1 or 5.0).** A new `--mqtt-protocol-version` option (config-file key `mqtt-protocol-version`; default `3.1.1`) selects whether the MQTT client speaks MQTT 3.1.1 or 5.0, so a deployment can match the dialect its broker and Home Assistant install expect. The active version is reported on `GET /api/diagnostics/mqtt`.

### Changed

- **MQTT client rebuilt on the async-mqtt library.** The hand-rolled MQTT engine is replaced by the maintained async-mqtt (Boost.Asio) library, which now owns wire framing, keep-alive, and packet encoding/decoding. MQTT 5.0 is fully supported as a result (previously 3.1.1 only). Reconnection with exponential backoff, the bounded drop-oldest publish queue, Last-Will, TLS with hostname verification, and Home Assistant auto-discovery are unchanged. Connecting, subscribing, and publishing were verified against a live broker on both protocol versions.

## [0.4.0-beta.1] - 2026-06-18

### Added

- **Spa-side remote button programming, redesigned.** Each spa-side remote (Dual Spa Switch / Spa Link) now appears as a visual keypad on its card in the *Emulated*/*Actual Devices* sections of Diagnostics, showing what every button is mapped to and its live indicator state. On an emulated remote you can press a key to act as that remote on the bus; on any remote whose button-to-switch mapping is known, you can reprogram a key's function inline — choosing only from the functions the pool controller can actually assign (the controller's own picker list). This replaces the separate spa-side section and its free-text assignment form.
- **Matter commissioning QR code.** The diagnostics page renders the Matter commissioning QR code, so the bridge can be paired by scanning rather than typing the manual pairing code.

### Fixed

- **OneTouch recovers from controller fault states.** When controller communications resume after a fault, an emulated OneTouch now returns to normal operation instead of remaining stuck in the fault state.

## [0.3.0-beta.3] - 2026-06-17

### Added

- **Chlorinator output setpoint in the web UI.** The dashboard now shows the chlorinator's *configured* output setpoint — distinct from the instantaneous generating percentage — with per-body pool and spa values where the controller reports them. The app keeps the figure current by proactively re-reading it (periodically and after a communications recovery) from the controller's Set AquaPure menu, and it is exposed on `/api/equipment` and in the OpenAPI spec.

### Fixed

- **Web-UI equipment toggles now reliably reach the controller.** Clicking an equipment button while emulating a OneTouch and/or RS Serial Adapter could appear to succeed while nothing actually changed. The RS Serial Adapter now sends the aux on/off command in the byte order the controller expects; an adapter the master is not polling — or a controller still starting up or in a fault state — no longer silently swallows the command, but instead routes to a controller that can act (or reports an honest failure). The web UI no longer shows a premature "Applied": a command stays pending until the controller confirms the new state, or it times out.

## [0.3.0-beta.2] - 2026-06-17

### Changed

- **Prerelease Docker images are tagged `edge`.** Stable releases continue to move the `latest` tag on GHCR; prereleases now move a rolling `edge` tag (in addition to the exact `<version>` tag), so you can track the newest prerelease with `ghcr.io/<owner>/aqualink-automate:edge`.

### Fixed

- **Trends chart renders when the page opens.** The Trends graph could appear blank when the app opened directly on that view, because the chart tried to draw before its panel had been laid out. It now draws as soon as the panel has a size, and redraws on window resize.
- **Trends selections survive a reload.** The chosen metrics, time range, and the show-inactive toggle are remembered across page reloads.

## [0.3.0-beta.1] - 2026-06-17

### Added

- **Show the aux id alongside the friendly name.** A new `show_aux_id_in_label` preference (off by default) renders device names as `Pool Light (Aux5)` in the web UI. The canonical label used for command dispatch, MQTT, and Home Assistant is unchanged.

### Changed

- **Trends page redesign.** The Trends view is rebuilt around faceted, vertically-stacked panels that share one time axis and a synchronized hover crosshair — a temperature axis, an auto-scaled water-chemistry overlay, and an equipment-runtime timeline (on/off bars reconstructed from transitions) — with per-window stats (now / min / max / avg) and grouped metric pickers. Device history is keyed by the equipment's stable UUID and carries its friendly label, so a device renamed after discovery (e.g. `Aux5` → `Pool Light`) is a single series rather than two; legacy duplicate series are folded in automatically.
- **Stable auxiliary identities.** Auxiliaries are assigned a deterministic identity derived from their hardware aux id instead of a random one regenerated each run, so a device keeps the same identity across restarts and reinstalls. Aux-id labels are also parsed consistently — `Aux5`, `Aux 5`, and a friendly name like `Pool Light` all resolve to the same device for control.

### Fixed

- **Auxiliaries no longer duplicate after a restart.** Equipment restored from the cache at start-up is reconciled with live discovery by stable identity, so devices such as multiple `Swim Jet` / `Spa Jet` outputs are no longer doubled-up on the dashboard after an upgrade or restart.

## [0.2.0-beta.4] - 2026-06-17

### Added

- **Jandy/Zodiac RS-485 integration.** Decode and control AquaLink RS panels over the RS-485 serial protocol.
- **Pentair RS-485 support.** Pentair equipment is decoded alongside Jandy/Zodiac: VSP/IntelliFlo pumps, IntelliChlor salt-water generators, and IntelliCenter/EasyTouch controllers. The protocol is auto-detected from the frame preamble, so a mixed or Pentair-only bus needs no extra configuration.
- **Spa-side remote support.** Dual Spa Switch and Spa Link spa-side remotes are decoded, emulated, and surfaced in the web UI (button presses and LED state).
- **Chlorinator / AquaRite support.** AquaPure and AquaRite salt-water generators report salt PPM, output percentage, and status, including boost mode.
- **Serial connectivity options.** Connect over a physical USB-to-RS485 adapter, or over the network to a remote serial server using RFC2217 or a raw TCP stream.
- **Record and replay.** Capture live serial traffic to a `.cap` file with `--record-serial`, then replay it offline. Recording can also be started and stopped at runtime from the diagnostics view.
- **Matter (smart-home) bridge.** Expose the pool equipment to Apple Home, Google Home, Amazon Alexa, and Samsung SmartThings over Matter. The bridge runs as a Node.js sidecar and is **on by default**; opt out with `--matter false` (config-file key `matter = false`). See [docs/MATTER.md](docs/MATTER.md).
- **MQTT integration with Home Assistant auto-discovery.** Publish system, pool, and per-body state to an MQTT broker, and have entities appear in Home Assistant automatically via the MQTT discovery payload. Command topics route back to the equipment.
- **Web UI.** An Alpine.js single-page app for monitoring and controlling equipment, served by the built-in HTTP server.
- **HTTP REST API and WebSocket protocol.** Read and control equipment over HTTP routes (under `/api/...`) and stream live updates over WebSocket. The OpenAPI spec is served at `GET /api/swagger.yaml`.
- **Health check endpoints.** `GET /api/health` is an unauthenticated **liveness** probe returning `{"status":"ok","uptime_seconds":N}`. The Docker image ships a `HEALTHCHECK` that polls it, so orchestrators (Docker, Kubernetes) can detect and restart a wedged container. It stays reachable without credentials even when `--api-auth-token` is set, and carries no sensitive data. A richer `GET /api/health/detailed` adds a **readiness** view — overall readiness (`200` when configured, `503` while starting), uptime, version, and per-subsystem checks (configuration + equipment validation, equipment count, MQTT connectivity); it is gated by the bearer-token policy since it exposes internal state.
- **Prometheus metrics.** Scrape runtime metrics from `/metrics` in the Prometheus text exposition format.
- **History database.** Enable a time-series store with `--history-db` and query it via `GET /api/history/series` (list series, or fetch bucket-averaged points for one series). The Trends page charts this history.
- **Scheduler.** Run time-based automation from a schedules file with `--schedules-file`, managed over a CRUD API: `GET`/`POST /api/schedules` (POST creates a schedule, returning `201` plus the new entity) and `GET/PUT/DELETE /api/schedules/{uuid}`.
- **Preferences and equipment cache.** Persist user preferences with `--preferences-file` (exposed at `/api/preferences`) and cache discovered equipment with `--equipment-cache-file`. When the respective file option is empty, the feature stays in memory only or disabled.
- **Alerting.** Raise alerts on low salt and serial communications loss, and POST every alert transition to a webhook with `--alert-webhook-url`.
- **Configurable container user (`PUID`/`PGID`).** The Docker image runs the app as a configurable uid:gid instead of a hard-coded `10000`. Set `PUID`/`PGID` (defaults `10000`); the entrypoint starts as root, remaps its user, chowns `/data`, then drops privileges to that user via `gosu`. `PUID=0` keeps it running as root. Bind-mounted read-only config still needs to be readable by the chosen uid on the host.

### Changed

- **API access control is opt-in.** The HTTP/WebSocket control plane can require a bearer token, restrict origins to an allow-list, and demand a CSRF header on state-changing requests. All of these are **off by default**, preserving the historical no-auth behavior unless you enable them (for example, by passing `--api-auth-token`).

### Security

- **The server binds to localhost by default and authentication is off.** With auth disabled, anyone who can reach the bound address can read and control your pool equipment. Enable `--api-auth-token` and bind only to trusted networks before exposing the server beyond localhost.
- **MQTT credentials are sent in cleartext without TLS.** Use TLS for any broker that is not on a fully trusted local network, and never use `--mqtt-tls-skip-verify` in production.
