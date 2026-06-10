# Connecting Aqualink-Automate to the Alwin32 simulators (no-install)

How the `Alwin32` simulators actually exchange traffic, what that means for capturing
fixtures, and a no-install harness that drives the **live app** with sim-derived `.cap`
fixtures. Companion to [testing-ecosystem.md](testing-ecosystem.md) (the generated/live fixture
model) and [pwrcntr-behavior.md](pwrcntr-behavior.md).

## How the simulators interconnect (decompiled from `NetIO.dll`)

The sims do **not** speak TCP and do **not** put their inter-sim traffic on a wire:

- `NetIO.dll` has a **`Shared` PE section** (VA `0x24000`) that holds the 16-slot device
  **net table**. Because the section is `IMAGE_SCN_MEM_SHARED`, every sim process that loads
  `NetIO.dll` maps the **same physical net table** — that shared table *is* the bus.
- `netSendMsg` (`NetIO.dll!0x1c60`) just writes a frame into the shared net table
  (`memcpy(slot+0xB0, data, 700)`, `slot+0xAE = len`); `netGetHWND` (`0x1c2b`) returns the
  consumer's `HWND` so a sim can `PostMessageA` it to say "new data is in the table". This is
  **shared memory + a window-message poke**, not a socket or a serialized stream.
- `NetIO.dll` imports **no winsock** (KERNEL32/USER32/WINMM only) — there is no TCP anywhere.
- `CommInit` (`NetIO.dll!0x2230`) opens a **real COM port** (`CreateFileA("\\.\COM%d")` +
  `SetCommState`). The physical serial wire is used **only to talk to real hardware** — e.g.
  the PowerCenter sim driving an RS-485 bus. (`CommTx` framing is documented in
  [../alwin32_simulator_protocol.md](../alwin32_simulator_protocol.md) §2.)

**Consequence for capture:** there is no no-install way to tap the *live* sim-to-sim bus — it
is in-process shared memory, and the only real wire (a COM port) cannot be shared with the app
without a virtual-serial driver. A true live-sim capture therefore needs **com0com** (see
[testing-ecosystem.md](testing-ecosystem.md) §B for the com0com + hub4com wiring).

## The no-install path that works: a raw-TCP feeder → the live app

The app *does* support a plain TCP serial transport (`--remote-serial-port host:port --rawtcp`,
`src/core/options/options_serial_options.h`). So we drive the **live app** with sim-derived
`.cap` fixtures over a socket — no driver, no GUI sims:

```
generate_fixtures.py  ->  *.cap  ->  tcp_feeder.py (TCP server)  ->  aqualink-automate (TCP client)
                                                                      full decode + DataHub + web/MQTT
```

`tcp_feeder.py` (next to the fixtures) serves a `.cap`'s R-bytes to the connecting app and logs
the app's writes. Verified end-to-end against the **live** debug build:

```
# terminal 1 -- start the feeder
python test/fixtures/alwin32/tcp_feeder.py test/fixtures/alwin32/chlorinator.cap --port 9099 --loop

# terminal 2 -- point the app at it (plain socket; --no-rfc2217 keeps it raw)
aqualink-automate --remote-serial-port 127.0.0.1:9099 --rawtcp --no-rfc2217 \
    --disable-https --address 127.0.0.1 --http-port 18080 --doc-root assets/web \
    --loglevel-messages info --loglevel-devices info
```

Observed (chlorinator.cap):
```
(Serial)    Successfully connected to network serial port: 127.0.0.1:9099
(Equipment) Adding new SWG device with id: 0x50
(Devices)   AquariteDevice: Creating chlorinator device from AquaRite wire data
```

i.e. the fixture fed over TCP populated a real chlorinator in the DataHub. Swap the `.cap` for
`epump.cap` / `heater.cap` / `iaq_page.cap` / `intelliflo.cap` to exercise other devices and
behaviours; add `--record-serial roundtrip.cap` to capture what the app sees. (With the app's
emulation enabled, the feeder's write-log captures the commands the app would transmit, so the
harness is bidirectional.)

**Net:** "generate `.cap` for specific devices/behaviours" is fully no-install today (the
generator + this feeder, driving the real app). "Capture the *live* simulators" needs com0com —
**now done and VERIFIED**, see next section.

## C. Live capture from the simulators (VERIFIED, com0com)

With a com0com virtual pair (here `COM3 <-> COM5`), the **PowerCenter master sim** drives the
bus and the app records it. Topology that works:

```
  Pwrcntr.exe  --Comm Port-> COM3   <--com0com-->   COM5  --serial-port--> aqualink-automate
  (master, model PD-8 Combo)                                 (emulates a device + --record-serial)
```

Steps: in `Pwrcntr.exe` set **Comm Port → COM3** (menu); run the app on **COM5** emulating a
device, e.g. OneTouch:
```
aqualink-automate --serial-port COM5 --record-serial pwrcenter.cap \
    --jandy-device-type OneTouch --jandy-device-id 0x41 \
    --disable-https --address 127.0.0.1 --http-port 18082 --doc-root assets/web \
    --loglevel-messages info
```
The master polls (`cmd 0x00` to a schedule of addresses, **with the leading/trailing `0x00`
pads on the wire** — confirming the §2 framing), the app's emulated OneTouch (0x41) Acks, and
the master then drives it with the **real classic-OneTouch display protocol**. A 30 s capture
(`test/fixtures/alwin32/pwrcenter_onetouch_live.cap`) decoded to genuine OneTouch screens:
`PD-8 Combo` (model), `REV T.0.1` (firmware), `Air 72°F` (the `` ` ``→° glyph), `Spa Mode OFF`,
the full menu tree (`Set Temp >`, `Diagnostics >`, …) — **live-validating** the model table
([pwrcntr-behavior.md](pwrcntr-behavior.md) §1) and the OneTouch opcodes ([onetouch.md](onetouch.md)).

**Gotchas learned:**
- **Don't run the equipment sims alongside a COM-mode master** — they share `NetIO.dll`'s
  in-memory net table and **deadlock** the COM-mode PowerCenter ("Not Responding"). Capture
  device traffic by having the *app* emulate the device (`--jandy-device-type`), not by running
  `Aquarite.exe`/`ePump.exe`. The valid emulation types are `OneTouch IAQ SerialAdapter`.
- **IAQ page protocol — now working.** The master discovers an AqualinkTouch (`0x30`–`0x33`)
  with a generic **Probe (`cmd 0x00`)**, exactly as it does a OneTouch — but `IAQDevice` only
  Ack'd `IAQMessage_Poll` (`0x30`), never the bare probe, so an emulated `0x33` stayed silent and
  the master never rendered pages. Fix: `IAQDevice` now has a **probe handler that Acks in
  emulated mode** (`Slot_IAQ_Probe`; no-op when passively decoding). With it,
  `--jandy-device-type IAQ --jandy-device-id 0x33` makes the master drive the **full IAQ page
  protocol**. Captured `test/fixtures/alwin32/iaq_pages_live.cap`, which **live-validates** the
  static page RE: `0x23` PageStart; `0x24` PageButton with **two labels** (`Filter`/`Pump`,
  `Pool`/`Heat`, `Aux1/2/3`); `0x25` PageMessage = **`[lineId][text]`** (one leading byte — e.g.
  line 1 = `72°`, `Air Temp`, `Pool Temp`); `0x2d` TitleMessage `AquaLink Touch`; `0x28` PageEnd.
  (The `0x25` one-leading-byte layout matches the app and corroborates the decompilation method
  behind the `0x26` two-byte finding.) **`0x26` TableMessage is still not directly captured live.**
  It is rendered for table/grid pages, so it needs the emulated AqualinkTouch driven off the home
  page. Driving navigation via the chlorinator command (`POST /api/equipment/chlorinator
  {"percentage":60}`) does make the emulated 0x33 transmit a nav sequence on the wire (Acks
  carrying `0x02` back, `0x19` open-AquaPure, `0x11` select, `0x80` submit), but the master then
  **stops rendering** rather than showing a table page: those `IAQ_CMD_*` bytes are the
  **iAqualink2 (0xA3) command-channel** vocabulary, not the AqualinkTouch (0x33) **page-button**
  navigation the sim expects, so they confuse the master. Live `0x26` confirmation therefore needs
  the app to support **page-protocol navigation** (select an on-screen `0x24` PageButton by index
  on the 0x33 channel) — a small feature, not yet present. Until then `0x26`'s two-byte layout
  rests on the static decompile + the live-confirmed `0x25` sibling. (Same remaining-work bucket:
  an ePump capture for the [epump.md](epump.md) reconciliation.)

## Advanced (optional, not built): a shared-memory tap

Because the sim bus is `NetIO.dll`'s shared net table, a small helper that `LoadLibrary`s
`NetIO.dll` (mapping the same shared pages) could *read* the net-table slots while the sims run
and re-emit reconstructed frames to the feeder's TCP socket — a genuine no-install live tap.
It reads device *state* rather than a byte stream, so reframing is approximate; noted here as a
future option rather than a recommendation.
