# aqualink-matter-bridge

A Matter (smart-home) bridge **sidecar** for
[aqualink-automate](../README.md). It runs as a small Node.js/TypeScript process
alongside the C++ application, consumes the existing HTTP + WebSocket equipment API,
and exposes the pool equipment as a Matter **Aggregator (bridge)** so it pairs with
**Apple Home, Google Home, Alexa and SmartThings** by scanning a QR code.

It does **not** link into the C++ build — it talks to it purely over HTTP/WS. This
keeps the MSVC/C++ build untouched and the runtime image small.

## How it fits together

```
 Matter controllers (Apple Home / Alexa / ...)
        │  Matter over IPv6 mDNS (UDP 5540/5353)
        ▼
 ┌──────────────────────┐    HTTP/WS     ┌─────────────────────────┐
 │  matter-bridge (node) │ ─────────────▶ │  aqualink-automate (C++) │
 │  @matter/main         │  /api/equipment │  RS-485 ⇄ pool hardware  │
 │  Aggregator + endpoints│  /ws/equipment  │                         │
 └──────────────────────┘                 └─────────────────────────┘
```

- `src/aqualink-client.ts` — REST (`GET /api/equipment`, `/api/equipment/devices`) for
  discovery + initial state; `/ws/equipment` for live updates; commands via
  `POST /api/equipment/buttons/{uuid}`, `/setpoints`, `/chlorinator`. Reconnects with
  backoff and degrades gracefully while the controller reports "starting".
- `src/device-map.ts` — pure mapping from the equipment JSON onto Matter device kinds.
- `src/bridge.ts` — the matter.js `ServerNode` + `Aggregator`, one bridged endpoint per
  device, cluster command handlers → command API, and persistence of fabric state.
- `src/status-server.ts` — serves the commissioning **QR payload + manual code** on
  `GET /matter/status` (the C++ `/api/diagnostics/matter` route fetches this for the UI).
- `src/index.ts` — wires it together and keeps the bridge live.

## Device mapping

| Aqualink device            | Matter device type        | Notes                                    |
| -------------------------- | ------------------------- | ---------------------------------------- |
| Pump                       | On/Off Plug-in Unit       | on/off only (VSP speed not a command yet)|
| Aux / Cleaner / Spillover / Sprinkler | On/Off Plug-in Unit | toggle                                 |
| Light                      | On/Off Plug-in Unit (v1)  | dimmable/colour pending JSON enhancement |
| Heater                     | Thermostat (heat)         | setpoint per body (pool/spa)             |
| Chlorinator (SWG)          | On/Off + Level Control    | level = generating %; boost = 101        |
| Pool / Spa / Air temp      | Temperature Sensor        | read-only                                |

## Configuration (environment)

| Variable                     | Default                  | Meaning                                  |
| ---------------------------- | ------------------------ | ---------------------------------------- |
| `AQUALINK_API_URL`           | `http://127.0.0.1:80`    | C++ API base URL                         |
| `AQUALINK_API_TOKEN`         | _(unset)_                | bearer token if `--api-auth-token` is set|
| `MATTER_PASSCODE`            | _auto-generated_         | commissioning passcode (see below)       |
| `MATTER_DISCRIMINATOR`       | _auto-generated_         | commissioning discriminator (see below)  |
| `MATTER_UNIQUE_ID`           | _auto-generated_         | stable Matter node id (see below)        |
| `MATTER_PORT`                | `5540`                   | Matter UDP port                          |
| `MATTER_STORAGE_PATH`        | `/data/matter`           | fabric/commissioning persistence (volume)|
| `MATTER_BRIDGE_STATUS_PORT`  | `8099`                   | status/QR HTTP port (for the C++ UI)     |
| `MATTER_ENABLED`             | `true`                   | read by the Docker entrypoint (opt-out)  |

### Commissioning credentials (passcode / discriminator / unique id)

The commissioning **passcode**, **discriminator** and node **unique id** are *not*
hardcoded. On first boot the sidecar generates cryptographically-random,
per-install values (Node's `crypto`, not `Math.random`) and **persists** them to
`bridge-credentials.json` under `MATTER_STORAGE_PATH` (the mounted volume), then reuses
them on every subsequent boot so pairing — and the node identity — survive restarts.

This matters for security: Matter commissioning happens over the LAN (UDP 5540 + mDNS)
and the container runs with host networking, so any device on the network that knows the
passcode can pair and control pool equipment. A fixed, publicly-known default (such as the
matter.js example `20202021`) would let anyone on the LAN commission the bridge — so there
is deliberately no default.

- The generated passcode is uniformly random in the Matter-valid range (`1..99999998`),
  excluding the spec-invalid trivial values and the `20202021` example.
- The discriminator is a random 12-bit value (`0..4095`); the unique id is a random UUID.
- You may pin any of these via the `MATTER_*` env vars (e.g. to migrate an existing
  pairing); an explicit value is persisted too, and the sidecar **refuses to start** if
  the resolved passcode is the well-known `20202021` example.
- The credentials file holds the pairing secret, so the bridged `MATTER_STORAGE_PATH`
  volume should be protected like any other secret material.

## Networking requirement (important)

Matter commissioning needs **IPv6 mDNS** (UDP 5540 + 5353). Docker bridge networking
breaks this, so the container **must run with `network_mode: host`** (see the repo
`docker-compose.yml`). The fabric state directory (`MATTER_STORAGE_PATH`) must be a
persistent volume so pairing survives restarts.

## Develop / test

```bash
npm ci
npm run build       # tsc -> dist/
npm test            # node:test over the pure logic (device-map + client)
npm run typecheck   # tsc --noEmit
npm start           # run dist/index.js
```

The `device-map` and `aqualink-client` unit tests have no matter.js dependency, so they
run fast and in CI without a Matter stack.
