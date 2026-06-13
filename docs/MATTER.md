# Matter (smart-home) support

aqualink-automate exposes the pool equipment to **Apple Home, Google Home, Amazon
Alexa and Samsung SmartThings** over [Matter](https://csa-iot.org/all-solutions/matter/),
so a single QR-code pairing covers every major ecosystem.

Matter is implemented as a **sidecar bridge** ([`matter-bridge/`](../matter-bridge/README.md)):
a small Node.js process that consumes the existing HTTP/WebSocket equipment API and
presents the equipment as a Matter *Aggregator*. It does **not** link into the C++
build — keeping the C++/MSVC build untouched and the image small.

```
 Apple Home / Google / Alexa / SmartThings
        │  Matter over IPv6 mDNS (UDP 5540 / 5353)
        ▼
 ┌───────────────────────┐   HTTP + WS    ┌──────────────────────────┐
 │  matter-bridge (node)  │ ─────────────▶ │  aqualink-automate (C++)  │
 │  @matter/main Aggregator│  /api/equipment │  RS-485 ⇄ pool hardware   │
 └───────────────────────┘  /ws/equipment   └──────────────────────────┘
```

## Cross-controller actuation (why this works on any rig)

Commands reach the RS-485 bus through a **capability-routed command dispatcher**:
whichever controller is running advertises what it can do (`DeviceActuator`,
`SetpointController`, `ChlorinatorController`, …) and the dispatcher routes to the
highest-priority one present. Three controllers can actuate equipment and heater
setpoints, in precedence order:

1. **Serial Adapter (RSSA, `0x48`)** — *High*. A direct, stateless command channel.
2. **OneTouch (`0x40–43`)** — *Low*. Drives the menu via the Navigator (equipment
   toggle = in-place *Select* on the Equipment ON/OFF row; setpoint = arrow-stepping
   the value on the Set Temperature page).
3. **AqualinkTouch / iAqualink2 (`0x33`)** — *Lowest*. Presses the on-screen
   `PageButton` matching the device by name (`0x11 + index`) for toggles, and uses the
   value-submit protocol (select field → `0x80` → control-data value) for setpoints +
   the chlorinator.

So a rig with *any* of these can be controlled, and the most reliable channel present
wins automatically. (PDA `0x60–63` advertises nothing → honest `NotSupported`.)

## Device mapping

| Aqualink device                         | Matter device type      | Notes                                    |
| --------------------------------------- | ----------------------- | ---------------------------------------- |
| Pump (single/two-speed)                 | On/Off Plug-in Unit     | on/off (VSP speed not a command yet)     |
| Aux / Cleaner / Spillover / Sprinkler   | On/Off Plug-in Unit     | toggle                                   |
| Light                                   | On/Off Plug-in Unit (v1)| dimmable/colour pending a JSON change    |
| Heater                                  | Thermostat (heat)       | setpoint per body (pool/spa)             |
| Chlorinator (SWG)                       | On/Off + Level Control  | level = generating %; boost = 101        |
| Pool / Spa / Air temperature            | Temperature Sensor      | read-only                                |
| pH / ORP / salt ppm                     | (omitted in v1)         | no faithful Matter type                  |

## Enabling / disabling

Matter is **on by default (opt-out)**.

- App flag: `--matter false` (or `matter = false` in the config file).
- Docker: `-e MATTER_ENABLED=false` (read by the container entrypoint).
- Tunables: `--matter-storage-path`, `--matter-status-port`, `--matter-passcode`,
  `--matter-discriminator` (passcode/discriminator `0` ⇒ the sidecar generates and
  persists random values).

## Networking requirement (Docker)

Matter commissioning uses **IPv6 mDNS (UDP 5540 + 5353)**, which Docker's bridge
driver does not forward. The container therefore **must run with host networking**:

```bash
docker run --network host -v aqualink-matter:/data/matter aqualink-automate
# or simply:
docker compose up      # compose sets network_mode: host + a matter-data volume
```

The fabric-state volume (`/data/matter`) makes pairing survive restarts. Host
networking is Linux-only; on Docker Desktop (macOS/Windows) disable Matter with
`MATTER_ENABLED=false`.

## Pairing

1. Open the web UI → **Diagnostics → Matter**. When the badge reads **"Ready to
   pair"** the QR code and an 11-digit manual pairing code are shown. (The code is
   also printed in the container logs on first boot.)
2. **Apple Home:** Home app → **+** → *Add Accessory* → scan the QR (or *More
   options…* → enter the manual code). Approve the "uncertified accessory" prompt
   (the bridge uses a test vendor ID).
3. **One other ecosystem** (pick one):
   - **Alexa:** Alexa app → *Devices* → **+** → *Add Device* → *Other* → *Matter* →
     scan the QR / enter the code.
   - **Google Home:** Home app → **+** → *Set up device* → *Matter-enabled device*.
   - **SmartThings:** app → **+** → *Add device* → *Partner devices / Matter* →
     scan the QR.
4. Each ecosystem joins its own fabric; the **Matter** panel's "Paired fabrics"
   count increments per ecosystem. Toggle a pool device from the controller app and
   confirm it actuates on the bus, and that controller-side changes reflect back.

## Verification status

| Check                                                                   | Status |
| ----------------------------------------------------------------------- | ------ |
| C++ unit + integration suites (incl. OneTouch setpoint, dispatcher, fixture replay) | ✅ green (1726 + 101 cases) |
| OneTouch toggle replay driven from `onetouch_equipment_toggle.cap`      | ✅ Navigator emits in-place Select |
| OneTouch setpoint edit model vs `onetouch_setpoint_edit.cap` hardware    | ✅ Select-enter → arrow-step ±1 → Select-commit; Pool=line2/Spa=line3; on-screen units |
| AqualinkTouch (IAQ) aux + setpoint vs `iaq_aux_setpoint.cap` hardware    | ✅ toggle = 0x11+index by name (Pool Light 0x1a, Spillway 0x1c); setpoint = value-submit '1'+value |
| Sidecar unit tests (device-map, API client)                             | ✅ green (11 cases) |
| Sidecar typecheck + build (matter.js bridge)                            | ✅ `tsc` clean |
| Sidecar boots → emits a valid commissioning QR + manual code            | ✅ verified |
| `/api/diagnostics/matter` (enabled / opt-out / unreachable-sidecar)     | ✅ verified live on a replay fixture |
| Docker `matter-builder` stage builds + prod-pruned sidecar boots        | ✅ verified (node:22-bookworm-slim) |
| Docker runtime stage (NodeSource + tini on ubuntu) + entrypoint         | ✅ verified: both processes supervised, opt-out, SIGTERM, `--version` |
| Full `docker compose up` of the runtime image                           | ⚙️ runs in CI (`docker-verify`) — the in-container C++ build needs a populated `deps/vcpkg`, which the local reuse-vcpkg worktree intentionally omits |
| Physical pairing from Apple Home + a second ecosystem                   | 🔌 manual — requires hardware on the LAN (steps above) |
