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
generator + this feeder, driving the real app). "Capture the *live* simulators" specifically is
the one piece that needs com0com.

## Advanced (optional, not built): a shared-memory tap

Because the sim bus is `NetIO.dll`'s shared net table, a small helper that `LoadLibrary`s
`NetIO.dll` (mapping the same shared pages) could *read* the net-table slots while the sims run
and re-emit reconstructed frames to the feeder's TCP socket — a genuine no-install live tap.
It reads device *state* rather than a byte stream, so reframing is approximate; noted here as a
future option rather than a recommendation.
