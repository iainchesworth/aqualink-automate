# iAqualink2 / JandyLink (0xA0–0xA3) — boot init handshake

## Status & caveats

Reverse-engineered from a **single** live controller power-cycle capture
(`test/fixtures/iaq_boot_sequence.cap`). It is **undocumented** in the Jandy RS-485 spec and
in AqualinkD — and AqualinkD is itself a reverse-engineering project that logs these same
commands as `Unknown`, so it is **not** an authority for their meaning.

> **Read this table as: byte STRUCTURE = observed fact; semantic LABEL = working hypothesis.**
> The meanings below are inferred only from what is visible on the wire in one capture, on one
> unit. Confirm against more captures / a second iAqualink2 before trusting any label.

## Discovery: configured, not range-probed

At a cold boot the master runs a device-discovery probe (cmd `0x00`) across a fixed set of slot
addresses:

```
0x08–0x0b  0x10  0x18  0x20–0x2b  0x30–0x33  0x38–0x3b  0x40–0x43
0x48  0x50–0x53  0x58  0x60  0x68–0x6b  0x70–0x73  0x80–0x81  0x88–0x89  0xa3
```

It probes **`0xa3` but NOT `0xa0` / `0xa1` / `0xa2`**. The iAqualink2 is addressed only at the
slot the panel is configured for (here `0xa3`).

**Implication:** an emulated iAqualink2 is polled *only* if the **Aqualink panel** is configured
to expect one at that address. The master does not auto-discover one by scanning the
`0xA0–0xA3` range. (The earlier `iaq_onetouch_startup.cap` capture missed this because it began
after the master had already booted.)

## Init sequence (master → 0xa3, once at boot, before heartbeats)

| cmd  | payload (observed)               | structure (FACT)                  | meaning (HYPOTHESIS)                 |
|------|----------------------------------|-----------------------------------|--------------------------------------|
| 0x00 | — (none)                         | probe                             | standard Jandy "are you there?" probe |
| 0x61 | `00 00 00 01 00 1d`              | 6 bytes, binary                   | session/config init — **UNKNOWN**    |
| 0x50 | `20 20 20 20 20 20 20 20 20 00`  | 10-byte NUL-terminated ASCII (blank) | a name/label field, empty here — *tentative* |
| 0x51 | `"1BA62825B7C69A4C" 00`          | NUL-terminated ASCII, 16 hex chars | device serial / identity — *tentative* |
| 0x59 | `01`                             | 1 byte                            | flag — **UNKNOWN**                   |
| 0x52 | `..`                             | 1 byte                            | flag — **UNKNOWN**                   |
| 0x53 | …                                | heartbeat (repeats ~1/sec)        | poll / keepalive (master → device)   |

## Heartbeat (steady state)

`0x53` master → device, ~1/sec. The device replies (our emulation answers with an ACK,
type `0x1f` / cmd `0x00`). Per AqualinkD `iaqualink.c` (also RE'd): after a number of beats the
device requests MainStatus, which returns as `0x70` / `0x73`.

## Decoded in code so far

- `IAQMessage_DeviceId` (`0x51`) — parses the NUL-terminated ASCII id string. **Structure is
  solid; the "serial/identity" label is a working hypothesis** (named for the content, not from
  any authoritative source).

## TODO

- Decode `0x50` / `0x61` / `0x59` / `0x52` structurally, with **no invented semantics**.
- Emulation: a panel-configured virtual iAqualink2 must answer the probe (`0x00`), the init
  messages (`0x50`/`0x51`/…), and the heartbeat (`0x53`) to be adopted. Requires the
  panel-config step on the controller + iteration against the live wire.
- Confirm every field meaning above against additional captures / a different unit.
