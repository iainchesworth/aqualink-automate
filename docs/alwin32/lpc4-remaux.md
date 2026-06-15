# Lpc4.exe & Remaux.exe — Laminar Pulse Controller and Auxiliary Power Center protocols

Reverse-engineered from the official Jandy *Alwin32* simulator suite
(`C:\Program Files (x86)\Alwin32\Lpc4.exe`, `Remaux.exe`; debug MFC builds,
VS2003 / `mfc71d.dll`). Method: static x86 disassembly (pefile + capstone, via
`alw.py`). Every claim is cited as `Lpc4.exe!0xRVA` / `Remaux.exe!0xRVA` /
`NetIO.dll!0xRVA`. Both images have base `0x400000`, so VA `0x40xxxx` == RVA `0xxxxx`.

Cross-checked against the shared-protocol template `docs/alwin32_simulator_protocol.md`
(frame = `NetIO.dll!CommTx`; addressing = `(class<<3)|instance`; device model =
`RemoteLogOn` / `GetMasterMessage` / `RemoteStatus`).

**Ghidra cross-check (Remaux):** the Remaux command dispatch (`FUN_004017f3`), the relay
bitmask builder (`FUN_004015d2`), and the 8-bit/16-bit relay setters (`FUN_004016c8` /
`FUN_00401755`) were decompiled (project `ghidra-proj/alwin32` → `alwin32-re/decomp-remaux`)
and **confirm every claim below exactly**: cmd `0x00` poll → 3-byte status; `0x02` 6-byte bulk;
`0x08` set 16-bit (`idx=data[1]`, `val=data[2..3]` LE) + clear 8-bit; `0x09` set 8-bit
(`idx=data[1]`, `val=data[2]`) + clear 16-bit; then recompute bitmask + send status. The
bitmask packs bit `N-1` for relay `N` set iff its 8-bit slot OR 16-bit slot is non-zero.

**Legend:** `[CONFIRMED]` = directly observed in code/data. `[INFERRED]` = deduced from
data flow, flagged where the wire bytes themselves were not pinned down. Neither binary
embeds a PDB path; identity rests on resource strings.

---

# Part 1 — `Lpc4.exe` : LPC4 Laminar Pulse Controller

A **laminar-jet pulse/effect controller**. The device is a pure **receive-only** sink: it
takes one master command that selects a *pulsing effect* and displays it. It never
transmits a status frame.

## 1.1 Identity `[CONFIRMED]`

* Resource strings: `LPC4 MFC Application`, `LPC4`, document class `CLpc4Doc` /
  `CLpc4View` (`Lpc4.exe!0x2a79c`, `0x2a818`, `0x2362a0`-region).
* The status-line field is labelled **"Command from RS System"** (`Lpc4.exe!0x2a68a`) —
  i.e. the UI shows whatever effect the RS master commanded.
* Effect-name table (`Lpc4.exe`):
  `NO COMMAND` (`0x4250b8`, initial), `FAST PATTERN` (`0x4250cc`),
  `SLOW PATTERN` (`0x4250dc`), `RANDOM SINGLE PULSE` (`0x4250ec`),
  `RANDOM GROUP PULSE` (`0x425100`), `PULSE EVERY 1 SEC` (`0x425114`),
  `PULSE EVERY 4 SEC` (`0x425128`), `PULSE EVERY 8 SEC` (`0x42513c`),
  `NO PULSING` (`0x425150`), `??????` (`0x4250c4`, out-of-range).

## 1.2 NetIO imports — receive-only `[CONFIRMED]`

`Lpc4.exe` imports only **`GetMasterMessage`, `RemoteLogOn`, `RemoteLogOff`**
(`Lpc4.exe!info imports`). It does **not** import `RemoteStatus` (so it never sends a
device→master frame) nor `RemoteGetNextAddress` (address is hard-coded). Base Jandy
framing (un-suffixed NetIO symbols), not Pentair `_iFloVS` / `_ePumpAc`.

## 1.3 Address `[CONFIRMED]`

Hard-coded — **address byte `0x90`**, stored into the device object at `+0xBD` in the
constructor: `mov byte [esi+0xbd], 0x90` (`Lpc4.exe!0x40126b`). That same `+0xBD` byte is
then passed as the `addr` argument to both `RemoteLogOn` (`Lpc4.exe!0x4012c3`) and
`GetMasterMessage` (`Lpc4.exe!0x401351`).

Per the addressing model `addr = (class<<3)|instance`: `0x90` → **class `0x12`, instance 0**.
`[INFERRED]` (decomposition of the literal; the sim never calls `RemoteGetNextAddress`, so
the class number is only implied by the fixed address).

* **Log-on init buffer** = device object `+0xB4` (`Lpc4.exe!0x4012bc`,
  `RemoteLogOn(handle=[+0x1c], addr=[+0xBD]=0x90, initBuf=+0xB4)`). No explicit
  initialisation of `+0xB4..+0xBC` was found in code — left zero by allocation.
  `[INFERRED]`

## 1.4 Inbound (master → device) commands `[CONFIRMED]`

`GetMasterMessage(0x90, outBuf=+0x80)` is polled (`Lpc4.exe!0x401366`); the dispatch is a
single command test (`Lpc4.exe!0x401379`):

```
cmp byte [outBuf+0], 0x11      ; only command 0x11 is acted on
mov al,  [outBuf+1]            ; effect index (payload byte)
cmp al,  [+0xBE]              ; dedup vs last value (skip if unchanged)
movsx eax, al ; cmp eax,7 ; ja ->"??????"
jmp [eax*4 + 0x4013f0]         ; 8-way jump table on the effect index
```

| cmd | meaning | payload | note |
|----:|---------|---------|------|
| `0x11` | **Set pulse effect / mode** | `data[1]` = effect index (see table) | only command recognised |

Any command byte other than `0x11` is ignored (`Lpc4.exe!0x401382 jne`). `+0xBE`
(initialised to `0xFF`, `Lpc4.exe!0x4012e2`) holds the previous effect index so an
unchanged value is skipped.

### Effect-index decode — jump table `Lpc4.exe!0x4013f0` `[CONFIRMED]`

Verified by reading the 8 dword entries of the table:

| `data[1]` | effect displayed |
|----------:|------------------|
| `0x00` | NO PULSING |
| `0x01` | PULSE EVERY 8 SEC |
| `0x02` | PULSE EVERY 4 SEC |
| `0x03` | PULSE EVERY 1 SEC |
| `0x04` | RANDOM GROUP PULSE |
| `0x05` | RANDOM SINGLE PULSE |
| `0x06` | SLOW PATTERN |
| `0x07` | FAST PATTERN |
| `≥0x08` | `??????` (out of range, `Lpc4.exe!0x4013d3`) |

(Jump-table targets: index0→`0x40139b` … index7→`0x4013cc`; `Lpc4.exe!0x40139b`-`0x4013d8`.)

## 1.5 Outbound (device → master) `[CONFIRMED]`

**None.** `RemoteStatus` is not imported and is never called. The LPC4 is a passive
command receiver only — it has no salinity/level/fault data to report.

---

# Part 2 — `Remaux.exe` : Auxiliary Power Center (remote relay/aux controller)

A **remote power center** that drives 8 auxiliary relays. The master sets relay outputs;
the device reports an 8-bit "which relays are on" bitmask back.

## 2.1 Identity `[CONFIRMED]`

* Resource strings: `REMAUX MFC Application`, `REMAUX`, document class `CRemAuxDoc` /
  `CRemAuxView` (`Remaux.exe!0x2b888`, `0x2b908`, `0x240ac`).
* Panel title **"AUXILLARY PWR CENTER by Jandy Industries"** (`Remaux.exe!0x2b492`,
  `0x2b4d6`) — *sic*, double-L spelling in the resource.
* UI fields: relay buttons **`Aux8`–`Aux15`** and **`Pump`** (`Remaux.exe!0x2b1e2`,
  `0x2b22a`, `0x2b24e`-`0x2b3f6`); **`Mode`** = `Auto` / `Service` / `Time-Out`
  (`Remaux.exe!0x2b206`, `0x2b41a`, `0x2b43e`, `0x2b466`); **`Relay Status`** /
  **`Relay:`** (`Remaux.exe!0x2b516`, `0x2b54a`); **`Dim Pwr:`** (`Remaux.exe!0x2b572`).
* So the modelled accessory is an **8-relay aux power center labelled Aux8..Aux15** with a
  dimmer-power ("Dim Pwr") concept and an Auto/Service/Time-Out mode selector.

## 2.2 NetIO imports `[CONFIRMED]`

Full device API: **`RemoteGetNextAddress`, `GetMasterMessage`, `RemoteStatus`,
`RemoteLogOn`, `RemoteLogOff`** (`Remaux.exe!info imports`) — base Jandy framing.

## 2.3 Address `[CONFIRMED]`

Negotiated via `RemoteGetNextAddress(class = 5)` (`push 5` at `Remaux.exe!0x40116a`,
immediately before `call RemoteGetNextAddress` at `Remaux.exe!0x4011cf` → IAT thunk
`0x401a9e`). The returned address is stored to the device object at `+0xE8`
(`Remaux.exe!0x4011d4 mov [esi+0xe8], al`) and used as the `addr` arg everywhere.

* **Class = 5** → base address `(5<<3) = 0x28`; instances 0–3 ⇒ **`0x28`–`0x2B`**.
  `[CONFIRMED]` (class literal) / `[INFERRED]` (the 0x28 base follows from the addressing
  formula in the template).
* **Log-on init buffer** = `{0x01, 0x00, 0x00}` — `RemoteLogOn(handle=[+0x1c],
  addr=[+0xE8], initBuf = +0xD3)` (`Remaux.exe!0x4013cf`-`0x4013e0`); `+0xD3..+0xD5` are
  initialised to `01 00 00` in the constructor (`Remaux.exe!0x4011e7`, `0x4011ee`,
  `0x4011f4`). This is the same buffer later used as the status response (§2.5).

## 2.4 Inbound (master → device) commands `[CONFIRMED]`

`GetMasterMessage([+0xE8], outBuf = +0xBC)` (`Remaux.exe!0x4017f3`); dispatch on
`outBuf[0]` (`Remaux.exe!0x40180a`-`0x40181f`, a `sub/dec; je` chain):

| cmd | meaning | payload | handler |
|----:|---------|---------|---------|
| `0x00` | **Poll / keep-alive** — just emit the 3-byte status | — | `Remaux.exe!0x4018d9` |
| `0x02` | **Bulk relay snapshot** — 6-byte all-relay state | `data[1..6]` (6 bytes) | `Remaux.exe!0x40188e` |
| `0x08` | **Set relay (16-bit value)** — "dim/level" path | `data[1]`=relay idx (1–8), `data[2..3]`=16-bit LE value | `Remaux.exe!0x401845` |
| `0x09` | **Set relay (8-bit value)** — "on/off" path | `data[1]`=relay idx (1–8), `data[2]`=8-bit value | `Remaux.exe!0x401825` |

Any other command byte falls through and is ignored (`Remaux.exe!0x40181f jne ->ret`).

### Relay set internals (cmd 0x08 / 0x09) `[CONFIRMED]`

The device keeps **two parallel per-relay value arrays** in its object, both indexed by
relay number 1–8:

* **8-bit array** at `+0x9c..+0xb8` — written by helper `Remaux.exe!0x4016c8(idx, val)`
  (switch on idx: 1→`+0xa0`, 2→`+0xa4`, 3→`+0x9c`, 4→`+0xa8`, 5→`+0xac`, 6→`+0xb0`,
  7→`+0xb4`, 8→`+0xb8`).
* **16-bit/"level" array** at `+0x7c..+0x98` — written by helper
  `Remaux.exe!0x401755(idx, val)` (switch on idx: 1→`+0x7c`, 2→`+0x80`, … 8→`+0x98`).

Behaviour:
* **cmd 0x09** (`Remaux.exe!0x401825`): `0x4016c8(idx, data[2])` (set 8-bit array) **and**
  `0x401755(idx, 0)` (clear the level array for that relay). I.e. a plain on/off-style set.
* **cmd 0x08** (`Remaux.exe!0x401845`): `0x401755(idx, word(data[2..3]))` (set the 16-bit
  level array) **and** `0x4016c8(idx, 0)` (clear the 8-bit array). I.e. the dimmer/level
  ("Dim Pwr") path with a 16-bit value. `[INFERRED]` that 0x08 == the dimmer path (from the
  16-bit width + the "Dim Pwr" UI field); the byte layout itself is `[CONFIRMED]`.

Both 0x08 and 0x09 then recompute the status bitmask (`call 0x4015d2`) and immediately send
the 3-byte status (`Remaux.exe!0x401863`-`0x40187c`).

* **cmd 0x02** (`Remaux.exe!0x40188e`): sends status first, then `memcmp`s the inbound
  6 payload bytes (`outBuf` vs object `+0xE2`, len 6, `Remaux.exe!0x4018b2`); if changed,
  copies them in (`0x402620`, len 6) and refreshes the UI (`0x40147b`). So **cmd 0x02 is a
  6-byte bulk relay-state image** pushed from the master. `[CONFIRMED]` layout = 6 bytes;
  exact per-byte meaning of the 6 bytes is `[INFERRED]` (a full relay snapshot).

## 2.5 Outbound (device → master) status `[CONFIRMED]`

Built at object `+0xD3` and sent by `RemoteStatus(addr, +0xD3, len=3, sendNow=1)` — all
three call sites use the same shape (`Remaux.exe!0x40187c`, `0x4018a0`, `0x4018eb`):

| field | offset | value |
|-------|:------:|-------|
| `data[0]` | `+0xD3` | **response command `0x01`** (constant; set in ctor `Remaux.exe!0x4011e7`, never changed) |
| `data[1]` | `+0xD4` | **relay on/off bitmask** (8 relays, see below) |
| `data[2]` | `+0xD5` | `0x00` (ctor-zeroed; cleared again on cmd 0x02, `Remaux.exe!0x4018d1`) — purpose unknown, always 0 in the sim |

**Response = `[0x01][relay_bitmask][0x00]`, length 3.**

### Relay bitmask (`data[1]` = `+0xD4`) `[CONFIRMED]`

Packed in `Remaux.exe!0x4015d2`: for relay index N (1–8), **bit (N-1) is set if that relay
is "on" in *either* array** (8-bit array `+0x9c…` OR 16-bit array `+0x7c…` non-zero).

| bit | mask | 8-bit-array slot | 16-bit-array slot | relay idx |
|----:|:----:|:----------------:|:-----------------:|:---------:|
| 0 | `0x01` | `+0xa0` | `+0x7c` | 1 |
| 1 | `0x02` | `+0xa4` | `+0x80` | 2 |
| 2 | `0x04` | `+0x9c` | `+0x84` | 3 |
| 3 | `0x08` | `+0xa8` | `+0x88` | 4 |
| 4 | `0x10` | `+0xac` | `+0x8c` | 5 |
| 5 | `0x20` | `+0xb0` | `+0x90` | 6 |
| 6 | `0x40` | `+0xb4` | `+0x94` | 7 |
| 7 | `0x80` | `+0xb8` | `+0x98` | 8 |

(The two halves of `0x4015d2` OR into the same byte, so a relay set via the 8-bit *or* the
16-bit path lights the same status bit.) Mapping relay index ↔ the `Aux8..Aux15`/`Pump`
labels (which of the 8 buttons is index 1) is **`[INFERRED]`** — not pinned from code.

---

## Summary cross-reference

| device | exe | address | inbound cmds | outbound (cmd / len) |
|--------|-----|:-------:|--------------|----------------------|
| LPC4 Laminar Pulse Controller | `Lpc4.exe` | `0x90` (fixed; class `0x12`) | `0x11` set effect (1 byte, 0–7) | none (receive-only) |
| Auxiliary Power Center | `Remaux.exe` | `0x28`–`0x2B` (class 5) | `0x00` poll · `0x02` 6-byte bulk · `0x08` set relay 16-bit · `0x09` set relay 8-bit | `0x01` / 3 = `[0x01][bitmask][0x00]` |

**Notable for aqualink-automate:** neither device is currently modelled. The LPC4's
single `0x11` "set effect" command (effect enum 0–7) at fixed address `0x90` and Remaux's
`0x08`/`0x09` relay-set + `0x01` 3-byte bitmask status at class-5 addresses (`0x28`+) are
concrete, capture-checkable byte layouts. Both ride standard Jandy DLE/STX framing
(`NetIO.dll!CommTx`), so the existing generator/factory stack applies — only new message
types/devices would be needed. The 6-byte cmd `0x02` payload and the relay-index↔Aux-label
mapping are the two pieces still wanting a live capture to confirm.
