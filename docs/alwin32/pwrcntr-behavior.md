# PowerCenter controller behaviour — models, config & the IAQ page protocol

Decompiled from `Pwrcntr.exe` (the RS PowerCenter master sim) in the Jandy *Alwin32* suite,
using Ghidra 12.1 (headless) over the `alw.py`-located addresses. This extends
[pwrcntr-master.md](pwrcntr-master.md) (the wire/command layer) with the controller's
**behavioural model**: how firmware, model and pool/spa configuration change what the
controller does, plus the **IAQ page-message field layouts**. Cited `Pwrcntr.exe!0xRVA`
(image base `0x400000`). **CONFIRMED** = read from decompiled C; **INFERRED** flagged.

Firmware identity: **Aqualink RS Rev T, "ver 222"** (`REV T.0.1`); config persists to
`EEPROM.DAT`. Source paths in the binary: `C:\AQUALINK\Rev T ver 222\Alwin32 VC++ 5.0\pwrcntr\`.

---

## 1. The model / configuration model — `ControllerType`

A single INI value **`ControllerType`** (default `0x7225`) is decoded once at startup by
`FUN_0041e5ba` (`Pwrcntr.exe!0x41e5ba`) into the controller's whole personality. The model
name comes from `FUN_0041e3e3` (`0x41e3e3`); the "panel kind" from `FUN_0041e505` (`0x41e505`).
Putting them together (all CONFIRMED):

| `ControllerType` | Model name | Aux relays (`DAT_558ed9`) | Kind (`DAT_558ee4`) | Bodies |
|:----------------:|------------|:-------------------------:|:-------------------:|:------:|
| `0x7225` | **RS-8 Combo** | 7 | 0 = Combo | pool+spa (shared) |
| `0x7226` | **RS-6 Combo** | 5 | 0 = Combo | pool+spa |
| `0x7227` | **RS-4 Combo** | 3 | 0 = Combo | pool+spa |
| `0x7228` | **RS-8 Only** | 7 | 1 = Only | pool only |
| `0x7229` | **RS-6 Only** | 5 | 1 = Only | pool only |
| `0x722a` | **RS-4 Only** | 3 | 1 = Only | pool only |
| `0x722b` | **RS-2/6 Dual** | 6 | 2 = Dual | 2 (`DAT_558ed8=2`) |
| `0x722e` | **RS-12 Combo** | 11 | 0 = Combo | pool+spa |
| `0x722f` | **RS-16 Combo** | 15 | 0 = Combo | pool+spa |
| `0x7230` | **RS-12 Only** | 11 | 1 = Only | pool only |
| `0x7231` | **RS-16 Only** | 15 | 1 = Only | pool only |
| `0x7232` | **RS-2/10 Dual** | 10 | 2 = Dual | 2 |
| `0x7233` | **RS-2/14 Dual** | 14 | 2 = Dual | 2 |
| `0x3fac8` | **PD-8 Combo** | 7 | 0 = Combo | iAqualink |
| `0x3fac9` | **PD-8 Only** | 7 | 1 = Only | iAqualink |
| `0x7234` | (PDA/AquaPalm) | — | — | falls back to `0x7225` (`FUN_004537dc` returns `0x7225`) |

**The three personality axes** (the globals that gate every behaviour):

- **`DAT_00558ed9` = aux-relay count** — `RS-N` exposes `N-1` aux relays plus the filter pump
  (RS-4→3, RS-6→5, RS-8→7, RS-12→11, RS-16→15; the `RS-2/N` dual models → `N` aux). This bounds
  how many `Aux1..AuxN` devices the controller presents and polls.
- **`DAT_00558ee4` = panel "kind"**: **0 = Combo (shared pool+spa)**, **1 = Only (pool only)**,
  **2 = Dual (two independent bodies)**. Drives spa logic, valve actuators and page layout.
- **`DAT_00558ed8` = body count** (1, or **2** for Dual); **`DAT_00558ee1`** = 2 for Dual else 0;
  **`DAT_00558ee6`** = `(kind != Dual)`. `DAT_00558ee2` = 1 for the PD-8/iAqualink range.
- `DAT_00558ee3` = display capacity = `min(7 or 6-if-Dual, aux count)`.

> **For aqualink-automate:** this is the controller's equipment-capacity + body model. A capture
> taken against one `ControllerType` only exercises that model's behaviour; the test matrix
> (§4 of [testing-ecosystem.md](testing-ecosystem.md)) should cover at least one **Only**, one
> **Combo** and one **Dual** to catch the body/spa-dependent branches.

## 2. Config-gated behaviours (examples found)

The decompiler's cross-references show the config globals fan out into behaviour. Two concrete,
verified examples:

- **Page opcode selection is config-dependent** (`FUN_0047c183`, `0x47c183`): the equipment-status
  page type is `0x2a` when **kind == Only** (`DAT_558ee4==1`) but `0x29` otherwise (Combo/Dual);
  and the page-list command is `0x23` when **Dual** (`DAT_558ed8==2`) but `0x24` otherwise. So the
  same logical screen produces *different wire bytes* per configuration. (This is why a decoder
  must treat `0x29`/`0x2a` and `0x23`/`0x24` as the same page family.)
- **Freeze protection** (`FUN_0040cc94`, `0x40cc94`, ~26 KB; behaviour from its strings + the
  `DAT_558ee4` branches at `0x40d…`): an assignable device set gets force-driven during a freeze.
  Confirmed rules (strings, all CONFIRMED present): turns **Aux ON** and re-checks on a timer
  ("Freeze Protection will turn Aux ON again in %d…"); **holds the pump ON** ("Cannot turn pump
  OFF while in Freeze Protection"); keeps **Solar ON** ("Solar will remain ON while Freeze
  Protected"); offers a **Spa override** ("override Freeze Protection with Spa"); and reports
  whether the **optional adjustable freeze sensor** is installed. The spa/pump handling branches
  on `DAT_558ee4` (kind), i.e. freeze behaves differently for Only vs Combo vs Dual.

## 3. The IAQ page protocol — field layouts (master → AqualinkTouch/iAQ)

The master drives the AqualinkTouch (class 6, `0x30`–`0x33`) / iAQ screen by building messages in
its global TX buffer `DAT_005587c8` and sending them with `netIO` (broadcast `dest=0xFF`, retry
instances 0–4; see [pwrcntr-master.md](pwrcntr-master.md) §1). Buffer layout: `+1` = u16 length,
`+3` = command byte, `+4…` = payload. Decoded builders (all CONFIRMED):

| cmd | builder | wire payload (after cmd) | meaning |
|----:|---------|--------------------------|---------|
| **`0x23`** | `0x46ef4e` | `[pageType]` — **`0x2a`=EquipmentStatus (<10 items), `0x5b`=large list (≥10)** | **PageStart** |
| **`0x24`** | `0x4554a4` | `[index][state][u16 field][label1\0][label2\0]` | **PageButton** (two labels) |
| **`0x25`** | `0x4553d4` | `[lineIndex][text\0]` (cmd taken from caller's struct[0]) | **PageMessage — short** (1 prefix byte) |
| **`0x26`** | `0x4556c7` | `[b1][b2][text\0]` | **PageMessage — long** (2 prefix bytes: line + attribute) |
| **`0x27`** | `0x45577c` | `[p1][p2][p3][p4][p5]` (5 fixed bytes) | page sub-message / metadata |
| **`0x28`** | `0x457460` | `[b1][b2][pageNo+'0' ASCII][b4][b5]` | **PageEnd** (carries an ASCII page-number digit) |

### Resolution of the "PageMessage `0x25` vs `0x26`" discrepancy

**Both exist — they are two PageMessage variants, not a mislabel.** `FUN_004553d4` is a generic
line sender that takes the command byte from its caller's struct and emits `[cmd][oneByte][text]`;
it is called with **`0x25`** for short status lines (`{0x25, 4, "Spa Temp"}` `0x456331`;
`{0x25, 3, "No devices are currently on"}` `0x46ef4e`; `"*** FREEZE PROTECT ***"` `0x46e8a4`).
`FUN_004556c7` always emits **`0x26`** with **two** prefix bytes before the text.

So:
- **`0x25`** = page text line with **one** leading byte (line index) — the project's existing
  `PageMessage 0x25` is **CONFIRMED correct** for this variant.
- **`0x26`** = page text line with **two** leading bytes (line + attribute/column) — a **second,
  richer PageMessage the master also emits**, which the project's decoder should also accept.

The master *transmits* `0x23/0x24/0x25/0x26/0x27/0x28`; `0x25` additionally appears as a local
comparison value on the receive path (the AqualinkTouch echoes/acks it). `TitleMessage 0x2d` was
**not** observed emitted by this firmware's page builders — it may be a newer-firmware/IAQ-side
message; flag for capture against the real panel.

> **For aqualink-automate:** accept **both** `0x25` (1 prefix byte) and `0x26` (2 prefix bytes)
> as page line messages, and treat `0x29`/`0x2a` as the same EquipmentStatus page family (the
> byte differs by `ControllerType` kind, §2). PageEnd `0x28` byte[2] is an ASCII page-number
> digit (`'0'`+n), not a binary count.

## 4. The other host/config interfaces (from the sibling sims)

These extend the controller picture and are documented in their own files:
- **iAqualink2 status protocol** — `UAPI Client.exe` emulates the iAqualink2 Wi-Fi module on
  `0x33`+`0xA3`; status is a **TLV** container (`[cmd][count]([tag16be][value])*`), cmds
  `0x82/0x85` request, `0x83/0x84` main-screen, `0x86` aux; temp = `byte − 20` (0 = N/A), pump
  state OFF/ON/**SWITCHING**, system type Pool-Spa/Pool-Only/Dual. See
  [uapi-client.md](uapi-client.md).
- **RS Serial Adapter** — `Seradptr.exe` exposes an ASCII `!KEYWORD value` / `?KEYWORD = value`
  / `#error` host protocol bridged to RS-485 GET/SET (`0x13`/`0x14`). See
  [pc-host-bridge.md](pc-host-bridge.md).
- **PC Docking Station** — `Pcdock.exe`, RS-485 class **`0x0B` @ `0x58`**, paged config-EEPROM
  read/write (`0x19`/`0x1a`). New device class for `jandy_device_types.h`.
