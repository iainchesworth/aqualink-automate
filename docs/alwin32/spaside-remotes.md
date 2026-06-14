# `2x4rem.exe` & `8button.exe` — Jandy AquaLink RS spaside-remote protocols

Reverse-engineered from the official Jandy *Alwin32* simulator suite
(`C:\Program Files (x86)\Alwin32\2x4rem.exe`, `8button.exe`; debug MFC builds,
VS2003 / `mfc71d.dll`). Method: static x86 disassembly (pefile + capstone, via
`alw.py`), **cross-checked with Ghidra decompilation** (project `ghidra-proj/alwin32`,
`BehaviorMap.java`/`ForceDecompile.java` → `alwin32-re/decomp-2x4rem` & `decomp-8button`).
Every claim is cited as `2x4rem.exe!0xRVA` / `8button.exe!0xRVA` / `NetIO.dll!0xRVA`. Both
images have base `0x400000`, so VA `0x40xxxx` == RVA `0xxxxx`. **The Ghidra pass corrected
the Spa Link inbound command bytes** (LED image is cmd `0x02`, not `0x01`; text is cmd `0x03`)
— see §2.4.

Cross-checked against the shared-protocol template `docs/alwin32_simulator_protocol.md`
(frame = `NetIO.dll!CommTx`; addressing = `(class<<3)|instance`; device model =
`RemoteLogOn(handle,addr,initBuf)` / `RemoteStatus(addr,buf,len,sendNow)` /
`GetMasterMessage(addr,outBuf)`). The two devices documented here are the AquaLink RS
**spaside remotes** — keypads mounted at the spa for local control. They are the inverse
of receive-only equipment sinks: they *receive* LED/indicator state **from** the master
and *send* button presses **back to** the master.

**Device mapping** (user-confirmed):

* **`2x4rem.exe` = "Dual Spa Switch"** — a 2×4 spaside remote (2 columns × 4 rows = 8
  positions).
* **`8button.exe` = "Spa Link"** — an 8-button spaside remote.

> **Note on overlap with [`panels.md`](panels.md):** `panels.md` already documents these
> two binaries (plus `Smallcp.exe`) as generic "keypad/remote panels". This file is the
> **spaside-remote-focused** deep-dive under the user-confirmed product names, mirroring
> the [`lpc4-remaux.md`](lpc4-remaux.md) layout. The byte-level findings here were
> **independently re-derived** and agree with `panels.md` on every address, command, RVA,
> and status-frame field.

**Legend:** `[CONFIRMED]` = directly observed in code/data. `[INFERRED]` = deduced from
data flow, flagged where the wire bytes themselves were not pinned down. Neither binary
embeds a PDB path; identity rests on resource strings.

NetIO base-export RVAs referenced (`NetIO.dll!info`): `RemoteAddressUsed 0x1dd5`,
`RemoteLogOn 0x1dff`, `RemoteLogOff 0x1f2d`, `RemoteStatus 0x1fb5`,
`GetMasterMessage 0x2068`. **Neither binary imports `RemoteGetNextAddress`** → both use a
**fixed hard-coded base address** (verified by manual `RemoteAddressUsed` probing, §1.3 /
§2.3), exactly like LPC4.

---

# Part 1 — `2x4rem.exe` : "Dual Spa Switch" (2×4 spaside remote)

A spaside keypad with **8 buttons** (2 columns × 4 rows) and a small set of status
indicators. The master pushes which indicators to light; the device reports the index of
whichever button was pressed.

## 1.1 Identity `[CONFIRMED]`

* Resource strings: `2X4REM MFC Application` (`2x4rem.exe!0x2aa58` W), `2X4REM`
  (`0x2aad8` W), `2X4REM.EXE` (`0x2ab5c` W), about-box `About 2x4rem`
  (`0x2a71e` W) / `2x4rem Application Version 1.0` (`0x2a78a` W).
* Document/view classes `C2x4Doc` / `C2x4View` (`2x4rem.exe!0x2389a0`, `0x2389a8`).
* **Panel/window title `Remote 2x4`** (`2x4rem.exe!0x2d00a` W); status-bar field `Ready`
  (`0x2d05c` W), pane `SCRL` (`0x2d09e` W).
* No discrete per-button text labels are stored as resource strings — the 8 keypad
  positions are dialog push-buttons identified only by control id and a constant button
  index (§1.6); consistent with a plain 2×4 spaside switch.

## 1.2 NetIO imports `[CONFIRMED]`

Device API: **`GetMasterMessage, RemoteStatus, RemoteLogOff, RemoteLogOn,
RemoteAddressUsed`** (`2x4rem.exe!info imports`). **No `RemoteGetNextAddress`** → fixed
address. Base Jandy framing (un-suffixed NetIO symbols), not Pentair `_iFloVS` /
`_ePumpAc`.

## 1.3 Address — fixed base `0x10`, class `0x02` `[CONFIRMED]`

The address byte is **seeded to `0x10`** in the app-init routine and only its *instance*
nibble is bumped if the seed collides (a fixed base, not a negotiated class):

```
0x40115d  mov  byte [esi+0xc0], 0x10        ; seed candidate address = 0x10
0x401164  mov  al, [esi+0xc0]
0x40116b  call RemoteAddressUsed            ; is this address already taken?
0x401170  test ax, ax ; je 0x401194         ;  free -> keep it
;  taken -> advance the LOW 3 BITS ONLY (keep class, ++instance):
0x40117c  mov  cl, al ; inc cl ; xor cl,al ; and cl,7 ; xor cl,al
0x40118c  mov  [esi+0xc0], cl
0x401188  cmp  di, 0x10 ; jl 0x401164        ; probe up to 16 instances
```

(`2x4rem.exe!0x40115d`–`0x401192`.) The negotiated byte at the app object `+0xc0` is then
copied into the **document object at `+0xb9`** in the document constructor
(`0x4012a9 mov al,[eax+0xc0]` → `0x4012c5 mov [esi+0xb9], al`), and `+0xb9` is the `addr`
argument passed to **`RemoteLogOn`** (`2x4rem.exe!0x401332`, `addr=[esi+0xb9]`),
**`RemoteStatus`** (`0x40155d`), and **`GetMasterMessage`** (`0x401535`).

| field | value |
|------|------|
| seed / base address | **`0x10`** `[CONFIRMED]` (`mov byte [+0xc0],0x10` @`0x40115d`) |
| class = `addr >> 3` | **`0x02`** (`0x10 >> 3`) |
| instance range | `0x10`–`0x17` (low 3 bits probed) |

Per the addressing model `addr=(class<<3)|instance`: `0x10` → **class `0x02`, instance 0**.
`[CONFIRMED]` for the literal `0x10`; the class number `0x02` is the literal's
decomposition (the sim never calls `RemoteGetNextAddress`, so the class is implied by the
fixed base only). This matches the recovered map in `alwin32_simulator_protocol.md` §5.

* **Log-on init buffer** = device object `+0xb0` — `RemoteLogOn(handle=[+0x1c],
  addr=[+0xb9]=0x10, initBuf=+0xb0)` (`2x4rem.exe!0x401321`-`0x401332`). `+0xb0` is the
  same 3-byte buffer later used as the status response (§1.5): byte0 is seeded to `0x01`
  (`0x4012cd mov byte [esi+0xb0],1`), bytes `+0xb1`/`+0xb2` zeroed.

## 1.4 Inbound (master → device) commands `[CONFIRMED]`

`GetMasterMessage(addr=[+0xb9], outBuf=+0x7c)` (`2x4rem.exe!0x401535`); the returned
inbound length is stored to `+0xae`. Handler at `2x4rem.exe!0x40151f`:

1. **Always emit the 3-byte status first** — `RemoteStatus(addr, +0xb0, len=3, sendNow=1)`
   (`0x40155d`). So every poll returns the pending button (§1.5).
2. Read `bl = outBuf[0]` (the master command byte) and dispatch:
   `0x401568 dec eax; dec eax; jne 0x401595` → **only `cmd 0x02` is acted on**; every other
   command (including the `0x00` poll) just sends the status ACK.

| cmd | meaning | payload | handler |
|----:|---------|---------|---------|
| `0x00` (and any ≠ `0x02`) | **Poll / keep-alive** — ACK the 3-byte status only | — | default fall-through `0x401595` |
| `0x02` | **Set LED / indicator state** — 6-byte indicator image | `data[0..5]` (6 bytes) | `0x40156c` |

### cmd 0x02 internals — 6-byte LED image `[CONFIRMED]`

`memcmp(outBuf@+0x7c, cache@+0xb3, len=6)` (`0x402640` @`0x401576`); if changed,
`memcpy(+0xb3, +0x7c, 6)` (`0x402300` @`0x401586`) and redraw (`0x401420` @`0x401590`).
So the master pushes a **6-byte snapshot** (command byte + indicator data) which the panel
caches at `+0xb3`.

### LED indicator decode — 2 bits per indicator `[CONFIRMED]`

The redraw routine `2x4rem.exe!0x401420` (blink logic `0x401454`) decodes the cached LED
byte at `+0xb4` (= `data[1]` of the inbound 6-byte image) as **2 bits per indicator**:

| bits of `data[1]` (`+0xb4`) | indicator | UI ctrl id | states |
|:--------------------------:|:---------:|:----------:|--------|
| `[1:0]` | LED A | `0x3e9` | `0`=off · `1`=on · `2`/`3`=blink |
| `[3:2]` | LED B | `0x3ee` | `0`=off · `1`=on · `2`/`3`=blink |

Blink values (2/3) arm a 500 ms toggle timer (`SetTimer 0x1f4` style, `0x401480`-region).
The cached image is 6 bytes wide but this sim only has UI controls for **2 indicators**
(decoded from `data[1]`); the remaining cached bytes are not rendered. `[INFERRED]` that
real hardware may pack more indicators into the higher bytes — needs a live capture.

## 1.5 Outbound (device → master) status `[CONFIRMED]`

Built at object `+0xb0` and sent by `RemoteStatus(addr=[+0xb9], +0xb0, len=3, sendNow=1)`
(`2x4rem.exe!0x40155d`):

| field | offset | value |
|-------|:------:|-------|
| `data[0]` | `+0xb0` | **response command `0x01`** (constant; set in ctor `0x4012cd mov byte [esi+0xb0],1`, never changed) |
| `data[1]` | `+0xb1` | `0x00` (ctor-zeroed `0x4012b7`; never written elsewhere) |
| `data[2]` | `+0xb2` | **pressed-button code (1–8), `0x00` = none** — set by the button handlers, cleared after each send (`0x401595 and byte [esi+0xb2],0`) |

**Response = `[0x01][0x00][button_code]`, length 3.** The button code is cleared
immediately after each `RemoteStatus`, so a press is reported on exactly one poll.

## 1.6 Button set / model `[CONFIRMED]`

A single setter `2x4rem.exe!0x4015a4` writes its argument into `+0xb2`
(`mov al,[esp+4]; mov [ecx+0xb2],al`). **Eight** constant-pushing wrappers (one per keypad
position) call it — confirming an **8-button (2×4) spaside keypad**:

| button | code | wrapper RVA |
|:------:|:----:|:-----------:|
| 1 | `0x01` | `2x4rem.exe!0x4015b1` |
| 2 | `0x02` | `0x4015b9` |
| 3 | `0x03` | `0x4015c1` |
| 4 | `0x04` | `0x4015c9` |
| 5 | `0x05` | `0x4015d1` |
| 6 | `0x06` | `0x4015d9` |
| 7 | `0x07` | `0x4015e1` |
| 8 | `0x08` | `0x4015eb` |

So a "Dual Spa Switch" button press goes on the wire as `[0x01 0x00 <btn 1..8>]` from
address `0x10`. The mapping of button index ↔ physical function (which of the 8 keys is
Pool/Spa/Aux/etc.) is **`[INFERRED]`** — not labelled in code; needs a live capture or
the controller's spaside-switch config.

---

# Part 2 — `8button.exe` : "Spa Link" (8-button spaside remote)

A spaside keypad with up to **9 button codes** (8 labelled keys + a 9th extra/menu key),
a set of 2-bit status indicators, and a small text/config inbound channel. Like the Dual
Spa Switch it receives LED state and reports button presses, but it has a slightly richer
inbound vocabulary.

## 2.1 Identity `[CONFIRMED]`

* Resource strings: `8BUTTON MFC Application` (`8button.exe!0x2af20` W), `8BUTTON`
  (`0x2afa0` W), `8BUTTON.EXE` (`0x2b024` W), about-box `About 8button` (`0x2ab56` W) /
  **`8button Application Version 5.0`** (`0x2abc2` W).
* Document/view classes `C8buttonDoc` / `C8buttonView` (`8button.exe!0x23ca0`, `0x23cac`).
* **Panel/window title `8 Button Remote`** (`8button.exe!0x2d4d0` W; a leading control
  char makes the raw dump read `38 Button Remote`); doc strings `8butto Document`
  (`0x2d51a` W).
* As with the 2×4 remote, the keys are dialog push-buttons with constant button indices,
  not text-labelled resources (§2.6).

## 2.2 NetIO imports `[CONFIRMED]`

Device API: **`GetMasterMessage, RemoteStatus, RemoteLogOn, RemoteLogOff,
RemoteAddressUsed`** (`8button.exe!info imports`). **No `RemoteGetNextAddress`** → fixed
address. Base Jandy framing.

## 2.3 Address — fixed base `0x20`, class `0x04` `[CONFIRMED]`

Same manual-probe pattern as the 2×4 remote, but **seeded to `0x20`**:

```
0x401156  mov  byte [esi+0xc0], 0x20        ; seed candidate address = 0x20
0x40115d  mov  al, [esi+0xc0]
0x401164  call RemoteAddressUsed
0x401169  test ax, ax ; je 0x401194         ;  free -> keep
;  taken -> ++instance (low 3 bits) only:  cl=al; inc cl; xor cl,al; and cl,7; xor cl,al
0x40118c  mov  [esi+0xc0], cl
```

(`8button.exe!0x401156`–`0x401192`.) The negotiated byte is copied into the document
object at **`+0xbd`** (`0x4012d6 mov al,[eax+0xc0]` → `0x4012f2 mov [esi+0xbd], al`), and
`+0xbd` is the `addr` argument to **`RemoteLogOn`** (`8button.exe!0x4013cc`,
`addr=[esi+0xbd]`), **`GetMasterMessage`** (`0x4016cd`), and all three **`RemoteStatus`**
call sites (`0x401707`, `0x40172d`, `0x40177e`).

| field | value |
|------|------|
| seed / base address | **`0x20`** `[CONFIRMED]` (`mov byte [+0xc0],0x20` @`0x401156`) |
| class = `addr >> 3` | **`0x04`** (`0x20 >> 3`) |
| instance range | `0x20`–`0x27` |

`0x20` → **class `0x04`, instance 0**. Matches `alwin32_simulator_protocol.md` §5.

* **Log-on init buffer** = device object `+0xb4` — `RemoteLogOn(handle=[+0x1c],
  addr=[+0xbd]=0x20, initBuf=+0xb4)` (`8button.exe!0x4013be`-`0x4013cf`). `+0xb4` is the
  3-byte status buffer (§2.5); byte0 seeded `0x01` (`0x4012fa mov byte [esi+0xb4],1`).

## 2.4 Inbound (master → device) commands `[CONFIRMED — corrected via Ghidra]`

> **Command-byte correction (Ghidra decompilation of `FUN_004016b4`).** The first static
> read of the `dec`-chain placed the LED image at cmd `0x01` and text at cmd `0x02`. The
> Ghidra decompiler resolves the actual comparisons in the dispatch as `outBuf[0] == 0x02`
> (LED) and `== 0x03` (text). The corrected values are below — note this makes the LED image
> command **the same `0x02` as the Dual Spa Switch**, not a different byte.

`GetMasterMessage(addr=[+0xbd], outBuf=+0x80)` (`8button.exe!0x4016cd`); inbound length →
`+0xb2`. Handler `FUN_004016b4` dispatches on `outBuf[0]`:

| cmd | meaning | payload | handler |
|----:|---------|---------|---------|
| `0x00` (default, any ≠ `0x02`,`0x03`) | **Poll / keep-alive** — ACK 3-byte status only | — | else branch |
| `0x02` | **Set LED / indicator state** — 6-byte indicator image | `data[0..5]` (6 bytes) | `if (outBuf[0]==0x02)` |
| `0x03` | **Set text / config value** — string compare+copy path | `data[1..]` (text/value) | `else if (outBuf[0]==0x03)` |

> Every branch sends the 3-byte status (`RemoteStatus(addr, +0xb4, 3, 1)`) around its payload
> work; the pending-button byte `+0xb6` is cleared after each message.

### cmd 0x02 internals — 6-byte LED image `[CONFIRMED]`

`memcmp(outBuf@+0x80, cache@+0xb7, len=6)`; on change `memcpy(+0xb7, +0x80, 6)`
(`FUN_004025a0`) and redraw `FUN_00401483`.

### cmd 0x03 internals — text/config value `[CONFIRMED]`

Clears the pending-button byte `+0xb5`, sends status, then `strcmp(outBuf@+0x82, cache@+0xc0)`;
on change copies the inbound string into both `+0xc0` (`FUN_004118a9`) and `+0x7c` and
refreshes the UI (`FUN_00410c85`). I.e. a small **inbound text/config field** (a MFC `CString`
copy) distinct from the bitmap LED push. `[INFERRED]` exact semantic (likely a display/name string).

### LED indicator decode — 2 bits per indicator `[CONFIRMED]`

Redraw routine `8button.exe!0x401483` decodes the cached LED byte at `+0xb8`
(= `data[1]` of the cmd-`0x02` image) as **4 indicators × 2 bits**, then continues into
`+0xb9` for more:

| bits of `data[1]` (`+0xb8`) | indicator | UI ctrl id | states |
|:--------------------------:|:---------:|:----------:|--------|
| `[1:0]` | LED 1 | `0x3ee` | `0`=off · `1`=on · `2`/`3`=blink |
| `[3:2]` | LED 2 | `0x3ec` | same |
| `[5:4]` | LED 3 | `0x3e9` | same |
| `[7:6]` | LED 4 | `0x3f0` | same |

`data[2]` (`+0xb9`) bits `[1:0]` drive a further indicator with a blink timer
(`SetTimer 0x65` @`0x401508`), and the applier continues (`0x401514`+) over the remaining
cached bytes — i.e. the 8 indicator LEDs are spread across the 6-byte cache. Blink (value
2/3) arms a toggle timer. `[INFERRED]` exact byte→LED map beyond `data[1]` — needs a live
capture (do not over-encode from this binary).

## 2.5 Outbound (device → master) status `[CONFIRMED]`

Built at object `+0xb4`, sent by `RemoteStatus(addr=[+0xbd], +0xb4, len=3, sendNow=1)`
at all three call sites (`8button.exe!0x401707`, `0x40172d`, `0x40177e`):

| field | offset | value |
|-------|:------:|-------|
| `data[0]` | `+0xb4` | **response command `0x01`** (constant; set in ctor `0x4012fa mov byte [esi+0xb4],1`) |
| `data[1]` | `+0xb5` | `0x00` (ctor-zeroed; cleared again on cmd `0x03`; never set to data) |
| `data[2]` | `+0xb6` | **pressed-button code (1–9), `0x00` = none** — set by the button handlers, cleared after each send (`0x4017af and byte [esi+0xb6],0`) |

**Response = `[0x01][0x00][button_code]`, length 3.**

## 2.6 Button set / model `[CONFIRMED]`

Setter `8button.exe!0x4013d9` writes its argument into `+0xb6`
(`mov al,[esp+4]; mov [ecx+0xb6],al`). **Nine** constant-pushing wrappers call it
(table at `0x4017be`):

| button | code | | button | code |
|:------:|:----:|-|:------:|:----:|
| 1 | `0x01` | | 6 | `0x06` |
| 2 | `0x02` | | 7 | `0x07` |
| 3 | `0x03` | | 8 | `0x08` |
| 4 | `0x04` | | 9 | `0x09` |
| 5 | `0x05` | | | |

(Wrapper RVAs `0x4017be`,`0x4017c6`,`0x4017ce`,`0x4017d6`,`0x4017de`,`0x4017e6`,`0x4017ee`,
`0x4017f6`,`0x4017fe`.) Nine codes = the **8 named keys + a 9th extra key** (likely an
all-off / menu / select key). A "Spa Link" press goes on the wire as
`[0x01 0x00 <btn 1..9>]` from address `0x20`. Button-index ↔ physical-function mapping is
**`[INFERRED]`** — not labelled in code.

---

## Summary cross-reference

| device | exe | address | class | inbound cmds | outbound (cmd / len) |
|--------|-----|:-------:|:-----:|--------------|----------------------|
| Dual Spa Switch (2×4 spaside remote) | `2x4rem.exe` | `0x10`–`0x17` (fixed base `0x10`) | `0x02` | `0x00` poll · **`0x02`** set LEDs (6-byte image; 2-bit/indicator) | **`0x01`** / 3 = `[0x01][0x00][button 1–8]` |
| Spa Link (8-button spaside remote) | `8button.exe` | `0x20`–`0x27` (fixed base `0x20`) | `0x04` | `0x00` poll · **`0x02`** set LEDs (6-byte image; 2-bit/indicator) · **`0x03`** set text/value | **`0x01`** / 3 = `[0x01][0x00][button 1–9]` |

**Key model:** both are **base-Jandy DLE/STX** spaside remotes (no Pentair `0xA5`) at a
**fixed hard-coded address** (no `RemoteGetNextAddress`). Master→remote = a **2-bits-per-
indicator** LED image (off/on/blink) under inbound command **`0x02` on BOTH devices**
(Ghidra-confirmed; an earlier static read mis-placed the Spa Link's at `0x01`). The Spa Link
additionally accepts a `0x03` text/config string. Remote→master = a fixed **3-byte**
`[0x01][0x00][buttonCode]` frame where
`buttonCode` is a **small integer button index** (1..8 / 1..9), `0` when idle, reported on
the next poll and cleared after one send. This is the **spaside-remote button-press wire
encoding** — an index, not a bitfield.

**Notable for aqualink-automate:** neither spaside remote is currently modelled. The
address classes (`0x02`→`0x10`, `0x04`→`0x20`) extend `jandy_device_types.h`; the
button-press encoding (`[0x01][0x00][idx]`) and the 2-bit LED image are concrete,
capture-checkable byte layouts on the existing Jandy generator/factory stack.

## Real-world wiring topology — how spa-side switches reach (or don't reach) the bus

Validated against live hardware (RFC2217 bridge to a real Power Center) + the Jandy
**Surge Suppression Installation Manual, Sheet #6873 Rev. G** (figures 1 & 4), which document
the **Dual Spa Side Interface PCB, P/N 6588**. There are **two distinct wiring paths** for
spa-side switches, and only one is visible on RS-485:

| Spa-side switch | Wiring (per Sheet #6873) | On the RS-485 bus? | What the app sees |
|---|---|:---:|---|
| **Switch #1** | Hard-wired to the Power Center's **6-pin terminal bar** (RED/BLACK/GREEN/WHITE/BROWN/BLUE) — a direct analog connection | **No** | Nothing for the press; only the **equipment effect** via normal MainStatus/AuxStatus decode |
| **Switch #2 & #3** | Wired to the **Dual Spa Side Interface PCB (P/N 6588)**, which bridges them onto the **4-wire RS-485** bus (its 4-wire cable lands on the Surge Suppression PCB's red terminal bar) | **Yes** | A real bus device — the "Dual Spa Switch" / `2x4rem` at **0x10**, button presses as `[0x01][0x00][code]` |

So the `2x4rem` "Dual Spa Switch" we decode at **0x10 is the 6588 interface board**, representing
**switch #2 (button codes 1–4)** and **switch #3 (codes 5–8)** — the "2×4" = two 4-button switches.
Switch #1 has no bus presence at all.

### Live-capture validation (real hardware, 2026-06-14)
- **Switch #2 confirmed on the bus:** pressing its 4 buttons produced `[0x01][0x00][1..4]` acks from
  0x10 (`captures/spaside_setup_nav.cap`) — our Phase-1 decoder matches byte-for-byte. `[CONFIRMED]`
- **Switch #1 confirmed invisible:** pressing it (3 buttons, then a clean single press of "Pool Light")
  produced **zero** RS-485 frames anywhere in the byte stream — yet the equipment toggled: the press of
  "Pool Light" flipped the controller's MainStatus + AuxStatus (`Pool Light` → on) ~5 s later, seen via
  normal status decode (`captures/spaside_single.cap`). So switch #1's *effect* is visible, its *button*
  is not. `[CONFIRMED]`
- **Switch #3 codes 5–8** are `[INFERRED]` from the 2x4rem 8-code model + the 6588 "dual" design — not
  yet directly observed on this install (only switch #2 was pressed).

### Implications for aqualink-automate
- **Decode/emulate covers interface-board switches (#2/#3 via 0x10).** A user with the 6588 board gets
  full read + emulation + web control of those switches.
- **Switch #1 (direct-wired) is out of scope for remote decode/emulation by nature** — there is no bus
  frame to read or reproduce. Its equipment effects are already surfaced by the ordinary status decode,
  so to *drive* switch-#1 functions the app actuates the equipment through a controller (existing path).
- **Emulation models the 6588 board:** an emulated `DualSpaSwitch` at 0x10 represents **fake switch #2
  (press codes 1–4)** and **fake switch #3 (codes 5–8)** — lets users without the physical board add
  spa-side switches the controller honours.

## Spa-switch button assignment — config protocol over the bus (read + write)

Both controllers expose the spa-side switch button→function map AND let it be programmed over
RS-485 (RE'd from `captures/spaside_setup_nav.cap`; the maintainer navigated the config + changed
and reverted button 1:2). Stored controller-agnostically on the DataHub; see the Phase-4b commits.

### READ (implemented)
- **iAQ** ("Spa Remotes" page): assignment rows arrive as `IAQ_TableMessage` (0x26) group-0x00
  frames, text `"<switch>:<button>\t<function>"` (the tab + trailing NUL are sanitised to `'?'`).
  The function-picker is group-0x01. Menu path: Setup (page-button idx 21) → Spa Remotes (idx 6)
  → "4 Function Spa Switch".
- **OneTouch** ("Spa Switch" menu): the assignment list is a screen page titled `Spa Switch`
  (line 0) / `Button Setup` (line 1), rows `"<switch>:<button>   <function>"` (space-padded).
- One shared parser (`Utility::ParseSpaSwitchAssignmentLine`) handles both → `DataHub` map.

### WRITE

#### OneTouch — implemented (RE-verified)
**OneTouch keypress codes** (panel→master Ack, ack_type 0x80, command byte):
`0x04` = Select (enter page / confirm) · `0x05` = highlight-next (move down a list) ·
`0x06` = cycle value (step the function picker to the next option) · `0x02` = Back.

**Sequence to set Switch S, Button B → function F** (verified against the capture's 1:2 edit):
1. Navigate Program → … → **Spa Switch** (menu item).
2. Select (0x04) → "Spa Switch Setup" (the *number of switches* page: options 1/2/3).
3. Select (0x04) → "Button Setup" (the assignment list).
4. highlight-next (0x05) until the cursor is on row **S:B**.
5. Select (0x04) → the **"Button S:B"** function picker.
6. cycle (0x06) until the displayed function reads **F** — feedback-driven, exactly like the
   numeric SetTemperature value-step (compare on-screen text to F each step).
7. Select/Back to confirm; the list then shows `S:B  F`.

The function picker cycles a fixed list: All OFF, OneTouch 4h, Clean Mode, Spa Mode, Spillway,
Air Blower, Pool Light, Swim Jet, Spa Jets, Filter Pump, Spa, Pool Heat, Spa Heat, Solar Heat, …

Implemented as `OneTouchDevice::SpaSwitchEdit_ProcessStep` (a screen-driven `SpaSwitchEditGoal`,
sibling to the value-edit goal) — the *number-of-switches* page is passed with a **bare Select** so
the cursor never moves and the configured switch count is preserved. `OneTouchDevice` implements
`Capabilities::SpaSwitchConfigurator`; commit "Phase 4b write: OneTouch …".

#### iAQ — function-write MECHANISM decoded (`captures/iaq_spaswitch_edit.cap`)
The iAQ "Spa Remotes" UI is a page-button + table UI on the AqualinkTouch (0x33). Decoders:
`captures/decode_iaq_spaswitch.py` (overview), `captures/dump_iaq_window.py` (full frame dump of an
edit window). Commands are the page-RELATIVE press `0x11 + index` in 0x33's IAQ_Poll ACK; the master
pushes the page as `IAQ_PageButton` (0x24) named buttons + `IAQ_TableMessage` (0x26) rows where
`group 0x00` = the `S:B → function` assignments and `group 0x01` = the assignable-device PICKER list.

**Navigation (by NAME, generalizable via `FindPageButtonByLabel`):**
- Setup menu → press the **"Spa Remotes"** button → Spa Remotes page.
- Spa Remotes page: **idx 0** "4 Function Spa Switch", **idx 1** "8 Function SpaLink",
  **idx 2/3/4** = switch-COUNT selector "1"/"2"/"3".

**Function-edit (verified vs `iaq_spaswitch_edit.cap`, maintainer changed `1:2` Pool Light→Pool Heat→Pool Light):**
1. Open the **4-Function Spa Switch detail** (wire `0x16` in the capture) → renders the picker
   (`group 0x01`) + assignments (`group 0x00`, `1:2` = Pool Light).
2. **Select the `S:B` assignment row** (wire `0x17` for `1:2`).
3. **Scroll the picker** (`0x15`) until the target function sits at the **commit slot (picker row 3)** —
   feedback-driven exactly like the OneTouch value-step (read the `group 0x01` rows each step).
   (Pool Heat was already at slot 3 → no scroll; reverting to Pool Light needed one `0x15` scroll.)
4. **Commit** (`0x1f`) → the master writes the function and re-pushes `group 0x00` row `1:2` = the new
   function. Both the change and the revert used `0x1f` to commit whatever device was at picker slot 3.

So the iAQ model = open detail → select row → scroll picker so F is at the commit slot → commit, with
the picker/assignment `group` rows giving live feedback (same shape as the OneTouch executor).

**Index scheme — CROSS-VALIDATED across two buttons** (`1:2` in `iaq_spaswitch_edit.cap`, `2:2` in
`iaq_spaswitch_edit2.cap`; decoder `captures/decode_iaq_edit2.py`). On the 4-Function detail page
(`PageStart 0x3b`) the selectable cells form one contiguous page-button index space:
- **Assignment rows** (`group 0x00`, ordinal O = `(S-1)*4 + B`, so `1:1`=1 … `2:4`=8):
  **row-select page-button index = O + 4** → `1:1`=idx5, `1:2`=idx6 (`0x17`), `2:2`=idx10 (`0x1b`), …
- **Picker rows** (`group 0x01`, the 1-based visible row A = 1..7):
  **commit page-button index = A + 11** → slot1=idx12 (`0x1d`), slot3=idx14 (`0x1f`), …
- **Open detail** = press idx5 (`0x16`) from the Spa Remotes page (transitions `0x3a`→`0x3b`; idx5 is
  ordinal-1 / row `1:1`, so opening implicitly lands on row 1:1, then you select the real target row).
- **Scroll picker** = idx4 (`0x15`), pages the `group 0x01` list by one screen (≈7 items).
- The **commit press IS the save** — the master immediately re-pushes the changed `group 0x00` row.

Verified end-to-end both ways: `2:2` Swim Jet→Pool Heat = open(0x16) → select-row(0x1b,idx10) →
commit Pool Heat@slot3 (0x1f,idx14); revert = open → select-row(0x1b) → scroll(0x15) → commit Swim
Jet@slot1 (0x1d,idx12). `1:2` matches with row-select 0x17.

**General writer algorithm:** nav to Spa Remotes → open detail (0x16) → select row `O+4` →
read `group 0x01`; if target function F is visible at row A, commit `A+11`; else scroll (0x15) and
re-read (bounded) → confirm via the re-pushed `group 0x00` row = F. Feedback-driven on the decoded
picker/assignment table rows, same shape as the OneTouch executor.

**Implementation status:** until the iAQ executor is built, `IAQDevice::SetSpaSwitchAssignment`
returns `NotSupported` and the `SpasideRemoteController` **falls through** past the (Medium-priority)
iAQ to the (Low-priority) OneTouch — the fully-verified writer. When the iAQ writer lands, its Medium
rank takes precedence automatically.

#### Surface
Both controller paths route through `ISpasideRemoteController::SetButtonAssignment` +
`POST /api/equipment/spaside-remotes {action:"assign"}`. The requested mapping is persisted in
`PreferencesHub` so the UI reflects it.

### Flagged — needs a live RS-485 capture to confirm
* **Button-index ↔ physical-function map** (which of the 8/9 indices is Pool/Spa/Aux/Heat/
  etc.) — not labelled in either binary.
* **Full LED-image byte→indicator map** beyond `data[1]` — these sims only render the
  indicators they have UI controls for (2 on the Dual Spa Switch, 4+ on the Spa Link);
  the meaning of the higher cached bytes of the 6-byte image is unconfirmed.
* The Spa Link **cmd `0x03` text/config** field's exact semantic (display name vs config).
* The **response command byte for an *unsolicited* button event**: the sims only ever ACK
  a master poll, so `0x01` is the sim-confirmed response cmd; whether a real remote sends
  a different command byte when reporting a press without being polled was not observed.

### Provenance / re-derivation
* **2x4rem (Dual Spa Switch):** addr probe `0x40115d`; ctor addr-copy `0x4012c5`
  (`+0xc0`→`+0xb9`), status-cmd seed `0x4012cd`; logon `0x401332` (`addr=+0xb9,
  initBuf=+0xb0`); GetMasterMessage `0x401535` (`outBuf=+0x7c`); handler `0x40151f`;
  RemoteStatus `0x40155d` (`out=+0xb0,len=3`); cmd-0x02 cache `+0xb3`; LED applier
  `0x401420`/blink `0x401454`; button setter `0x4015a4`, wrappers `0x4015b1`+.
* **8button (Spa Link):** addr probe `0x401156`; ctor addr-copy `0x4012f2`
  (`+0xc0`→`+0xbd`), status-cmd seed `0x4012fa`; logon `0x4013cc` (`addr=+0xbd,
  initBuf=+0xb4`); GetMasterMessage `0x4016cd` (`outBuf=+0x80`); handler `0x4016b4`;
  RemoteStatus `0x401707`/`0x40172d`/`0x40177e` (`out=+0xb4,len=3`); cmd-0x01 cache `+0xb7`,
  cmd-0x02 text cache `+0xc0`; LED applier `0x401483`; button setter `0x4013d9`, wrappers
  `0x4017be`+.
