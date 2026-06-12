# Jandy keypad / remote panels — `2x4rem.exe`, `8button.exe`, `Smallcp.exe`

Reverse-engineered from the official Jandy *Alwin32* simulator suite
(`C:\Program Files (x86)\Alwin32`, debug MFC builds, VS2003 / `mfc71d.dll`).
Method: static x86 disassembly (pefile + capstone, via `alw.py`). Every claim is
cited as `<exe>!0xRVA` (all three exes have image base `0x400000`, so VA `0x40xxxx`
== RVA `0xxxxx`) or `NetIO.dll!0xRVA`.

Cross-checked against the shared-protocol template
`docs/alwin32_simulator_protocol.md` (§2 frame, §3 addressing, §4 the bidirectional
"network table"). These three are **remote panels**: they *receive* LED/display data
**from** the master (in the inbound `GetMasterMessage` buffer) and *send* button
presses **back to** the master (in their `RemoteStatus` payload).

**Legend:** `[CONFIRMED]` = directly observed in code/data. `[INFERRED]` = deduced
from data flow / not pinned to wire bytes; flagged.

NetIO base-export RVAs used below (`NetIO.dll!info`):
`RemoteGetNextAddress 0x1d15`, `RemoteAddressUsed 0x1dd5`, `RemoteLogOn 0x1dff`,
`RemoteLogOff 0x1f2d`, `RemoteStatus 0x1fb5`, `GetMasterMessage 0x2068`.

---

## 0. The common panel pattern

All three are the same shape — a thin MFC shell over `NetIO.dll` that:

1. **Claims a bus address** then `RemoteLogOn(handle, addr, initBuf)`.
2. On a UI timer, polls `GetMasterMessage(addr, inBuf)` → returns the **inbound length**;
   `inBuf[0]` is the **master command byte**, `inBuf[1..]` the payload.
3. Dispatches on `inBuf[0]`. Most commands carry **LED / display state** the master
   wants lit on the panel.
4. **Always** answers with `RemoteStatus(addr, outBuf, len=3, sendNow=1)` — a fixed
   **3-byte** payload `[respCmd][b1][buttonCode]`. The leading two bytes are constants
   seeded at construction; the **last byte is the pending button press** (0 = none),
   cleared after each send.

So for every panel the *protocol-relevant* facts are: **(a) bus address/class**,
**(b) the inbound command set + LED/display byte layout**, **(c) the 3-byte outbound
frame and its button code**. All three confirm the template's §4 model
(`RemoteStatus`/`GetMasterMessage` slot semantics), e.g. `Smallcp.exe!0x402568`
(`jmp [GetMasterMessage IAT]`) and `0x402562` (`jmp [RemoteStatus IAT]`).

### LED encoding (shared) — 2 bits per indicator `[CONFIRMED]`

Every panel's LED applier decodes an inbound LED byte **2 bits per LED**:
`val = (ledByte >> (2*i)) & 3`, where **0 = off, 1 = on, 2/3 = blink** (the sim tracks
a per-LED blink toggle counter and flips state on each blink frame).
Cited: `8button.exe!0x401483`, `2x4rem.exe!0x401420`/`0x401454`.

---

## 1. `2x4rem.exe` — "Remote 2x4" / Dual Spa switch

**Identity:** FileDescription "2X4REM MFC Application"; window title **"Remote 2x4"**
(`2x4rem.exe!0x2d00a` W); "About 2x4rem"; version 1.0. Status-bar field "Ready".
NetIO imports: `GetMasterMessage, RemoteStatus, RemoteLogOff, RemoteLogOn,
RemoteAddressUsed` (no `RemoteGetNextAddress` → address is probed manually).

### Address — class `0x02`, base `0x10` `[CONFIRMED]`
Manual instance probe (`2x4rem.exe!0x40115d`–`0x401192`):
* seed `addr = 0x10` (`mov byte [esi+0xc0], 0x10` @`0x40115d`).
* loop: `RemoteAddressUsed(addr)`; if free (`test ax,ax; je`) use it; else advance the
  **low 3 bits only**: `cl=al; cl++; cl^=al; cl&=7; cl^=al` (= keep class, ++instance),
  up to `di < 0x10` iterations (`@0x401176`–`0x401188`).
* `RemoteLogOn(handle=[+0x1c], addr=[+0xb9], initBuf=[+0xb0])` (`@0x40132f`).

| field | value |
|------|------|
| class | `0x02` (`0x10 >> 3`) |
| base address | **`0x10`** (instances `0x10`–`0x17`) |

### Inbound (master → panel) — `GetMasterMessage(addr=[+0xb9], inBuf=[+0x7c])` `[CONFIRMED]`
Handler `2x4rem.exe!0x40151f`. Returns length to `+0xae`; `inBuf[0]` = command.
The handler **first sends the 3-byte ACK** (`RemoteStatus` @`0x40155d`), then dispatches
on `bl = inBuf[0]`: `dec; dec; jne default` → **only `cmd 0x02` is special**; everything
else (incl. the `0x00` poll) just ACKs.

| cmd | meaning | payload handling | cite |
|----:|---------|------------------|------|
| `0x00` (and any ≠2) | **poll** | ACK 3-byte status only (returns pending button) | `0x401568`–`0x40156a` |
| `0x02` | **LED state** | `memcmp(inBuf@+0x7c, cache@+0xb3, 6)`; on change `memcpy` 6 bytes & redraw | `0x40156c`–`0x401590` |

**LED byte layout (cmd 0x02)** `[CONFIRMED]` — applier `2x4rem.exe!0x401420`,
blink logic `0x401454`. Decodes **2 indicators** from cached `+0xb4` (= `inBuf[1]`):

| bits of `inBuf[1]` | LED | UI ctrl | states |
|:------------------:|-----|:-------:|--------|
| `[1:0]` | LED A | `0x3e9` | 0 off / 1 on / 2,3 blink |
| `[3:2]` | LED B | `0x3ee` | 0 off / 1 on / 2,3 blink |

(2 LEDs ⇒ the "2×4 / dual-spa" panel has 2 status indicators. The cmd-2 cache is 6
bytes, but only `inBuf[1]` is decoded here — remaining cached bytes are unused by this
sim. `[INFERRED]` that real hardware may pack more.)

### Outbound (panel → master) — `RemoteStatus(addr=[+0xb9], out=[+0xb0], len=3, now=1)` `[CONFIRMED]`

Fixed **3-byte** frame `out = [+0xb0][+0xb1][+0xb2]`:

| byte | offset | value | cite |
|:----:|:------:|-------|------|
| `out[0]` | `+0xb0` | **`0x01`** (response cmd; set once at init) | `0x4012cd` `mov byte [esi+0xb0],1` |
| `out[1]` | `+0xb1` | `0x00` (init-cleared, never written elsewhere) | `0x4012b7` |
| `out[2]` | `+0xb2` | **button code (1–8), 0 = none** — cleared after each send | setter `0x4015a8`; clear `0x401595` |

**Button encoding** `[CONFIRMED]` — 8 dialog-button handlers each push a constant into
`SetButton(al) → [+0xb2]=al` (`2x4rem.exe!0x4015a4`):

| button | code | wrapper |
|:------:|:----:|:-------:|
| 1 | `0x01` | `0x4015b1` |
| 2 | `0x02` | `0x4015b9` |
| 3 | `0x03` | `0x4015c1` |
| 4 | `0x04` | `0x4015c9` |
| 5 | `0x05` | `0x4015d1` |
| 6 | `0x06` | `0x4015d9` |
| 7 | `0x07` | `0x4015e1` |
| 8 | `0x08` | `0x4015eb` |

So a 2x4rem press goes on the wire as `[0x01 0x00 <btn>]` from address `0x10`.

---

## 2. `8button.exe` — "8 Button Remote" / Simple AllButton

**Identity:** FileDescription "8BUTTON MFC Application"; window title **"8 Button
Remote"** (`8button.exe!0x2d4d0` W, leading control char shows as `38…`); "About
8button"; **Application Version 5.0** (`0x2abc2` W). NetIO imports: `GetMasterMessage,
RemoteStatus, RemoteLogOn, RemoteLogOff, RemoteAddressUsed` (no `RemoteGetNextAddress`).

### Address — class `0x04`, base `0x20` `[CONFIRMED]`
Same manual instance probe as 2x4rem but seeded `0x20`
(`8button.exe!0x401156` `mov byte [esi+0xc0],0x20`; advance `@0x40116f`–`0x401185`,
loop `di < 8`). `RemoteLogOn(handle=[+0x1c], addr=[+0xbd], initBuf=[+0xb4])` (`@0x4013cc`).

| field | value |
|------|------|
| class | `0x04` (`0x20 >> 3`) |
| base address | **`0x20`** (instances `0x20`–`0x27`) |

### Inbound (master → panel) — `GetMasterMessage(addr=[+0xbd], inBuf=[+0x80])` `[CONFIRMED]`
Handler `8button.exe!0x4016b4`. Length → `+0xb2`; dispatch on `eax = inBuf[0]`
(`@0x4016e4`): `dec; dec; je cmd2` / `dec; je cmd3` / else default.

| cmd | meaning | payload handling | cite |
|----:|---------|------------------|------|
| `0x00` (default, any ≠2,3) | **poll** | ACK 3-byte status only | `0x4016f5`–`0x40170c` |
| `0x02` | **LED state** | ACK; `memcmp(inBuf@+0x80, cache@+0xb7, 6)`; on change `memcpy` 6 bytes & redraw | `0x40176c`–`0x40179f` |
| `0x03` | **display text / config** | clears button `+0xb5`; ACK; `strcmp(inBuf text@+0x82, cache@+0xc0)`; on change copy into `+0xc0`/`+0x7c` & redraw | `0x401714`–`0x401765` |

**LED byte layout (cmd 0x02)** `[CONFIRMED]` — applier `8button.exe!0x401483`.
Decodes cached `+0xb8` (= `inBuf[1]`) as **4 LEDs × 2 bits**, plus more from `+0xb9`:

| bits of `inBuf[1]` | LED | UI ctrl |
|:------------------:|:---:|:-------:|
| `[1:0]` | LED 1 | `0x3ee` |
| `[3:2]` | LED 2 | `0x3ec` |
| `[5:4]` | LED 3 | `0x3e9` |
| `[7:6]` | LED 4 | `0x3f0` |

`inBuf[2]` (`+0xb9`) `[1:0]` drives a further blink/extra indicator (`0x401511` timer
`0x65`); the applier continues at `0x401514` for additional cached bytes → the 8 button
LEDs are spread across the 6-byte cache (`[INFERRED]` exact byte→LED map beyond byte 1).

### Outbound (panel → master) — `RemoteStatus(addr=[+0xbd], out=[+0xb4], len=3, now=1)` `[CONFIRMED]`

Fixed **3-byte** frame `out = [+0xb4][+0xb5][+0xb6]`:

| byte | offset | value | cite |
|:----:|:------:|-------|------|
| `out[0]` | `+0xb4` | **`0x01`** (response cmd; set at init) | `0x4012fa` `mov byte [esi+0xb4],1` |
| `out[1]` | `+0xb5` | `0x00` (init/cmd3-cleared, never set to data) | `0x4012e4`, `0x401714` |
| `out[2]` | `+0xb6` | **button code (1–9), 0 = none** — cleared after each send | setter `0x4013d9`; clear `0x4017af` |

**Button encoding** `[CONFIRMED]` — 9 handlers push constants into
`SetButton(al) → [+0xb6]=al` (`8button.exe!0x4013d9`), table at `0x4017be`:

| button | code | | button | code |
|:------:|:----:|-|:------:|:----:|
| 1 | `0x01` | | 6 | `0x06` |
| 2 | `0x02` | | 7 | `0x07` |
| 3 | `0x03` | | 8 | `0x08` |
| 4 | `0x04` | | 9 | `0x09` |
| 5 | `0x05` | | | |

(9 codes — 8 labelled buttons + a 9th, likely an "all-off"/extra key.) A press goes on
the wire as `[0x01 0x00 <btn>]` from address `0x20`.

---

## 3. `Smallcp.exe` — "Small Control Panel" / Simple OneTouch

**Identity:** FileDescription "SMALLCP MFC Application"; window title **"Small Control
Panel"** (`Smallcp.exe!0x319f2` W); "About Smallcp"; version 1.0.001. Setup-menu
strings **"RS-485 Port"**, **"Fn Buttons"**, **"Delay Override"** (`0x2ef34/0x2ef50/
0x2ef6a` W); LED display font **"QuickType Mono"** (`0x2f13a` W). This is the
**OneTouch-class** panel: it negotiates its address via `RemoteGetNextAddress`, and
also imports `CommInit/CommInUse/CommID/CommEnd` (full comm-stack control) plus a
companion `BTN32D20.dll`. It has the richest inbound vocabulary (a character LED matrix +
cursor + blink-range) — far beyond the simple all-button panels.

### Address — class `0x08`, base `0x40` `[CONFIRMED]`
`Smallcp.exe!0x40194d`: `push 8; call [RemoteGetNextAddress]` (thunk `0x402544` →
IAT `0x4318dc`). Class **`0x08` → addresses `0x40`–`0x43`** — the documented OneTouch
range (matches template §3). Confirms the project's OneTouch id block.

### Inbound (master → panel) — `GetMasterMessage(addr=[+0xba], inBuf=[+0x9a])` `[CONFIRMED]`
Handler `Smallcp.exe!0x40201f`. Length → `+0xad` (`+0xac` cleared). `inBuf[0]` = command
(`movzx eax,[edi]`, `edi=+0x9a`); `inBuf[1]` saved to a temp. Dispatch `@0x402072`:
`cmp 8 / je` … `sub`-chain (recovered):

| cmd | handler | meaning | cite |
|----:|:-------:|---------|------|
| `0x00` | `0x402131` | **poll** — copy pending button `+0x195` → `out[+0xb1]`, send 3-byte status, then clear button | `0x402131`–`0x402166` |
| `0x02` | `0x4020ff` | **LED indicator state** — test `inBuf[4]` bits `0x30`/`0xc0`, `inBuf[5]` bits `0x03`/`0x0c`; select temp/value source `+0x8c`/`+0x7c`/`+0x80` | `0x4020ff`–`0x40212c` |
| `0x04` | `0x4020cb` | **display string** — ACK; 7-segment-decode 16 chars at `inBuf+2` (`+0x9c`), then render with `inBuf[1]` as line/offset | `0x4020cb`–`0x4020f5`; seg decode `0x401e64`; render `0x401bf3` |
| `0x05` | `0x402099` | **toggle bit** — `out[+0xb0] = (~[+0xb0]) & 0x80`, send | `0x402099`–`0x4020c3` |
| `0x08` | `0x40216b` | **cursor / selected-item LED** — ACK; set highlighted index 0–15 from `inBuf[1]` | `0x40216b`–`0x40218c`; setter `0x401cdb` |
| `0x09` | `0x40226d` | **clear / all-off** — ACK; reset blink/LED state (`+0xc1/+0xc3` = 0xffff) | `0x40226d`–`0x402289`; `0x401bd9` |
| `0x0F` | `0x4021ee` | **blink range** — ACK; set blink start/end (`+0x23d`/`+0x23f`) over a span of display cells | `0x4021ee`–`0x40226b`; setter `0x401d48` |
| `0x10` | `0x4021ac` | **set display item** — ACK; update a single display field from `inBuf[1..3]` | `0x4021ac`–`0x4021e9`; `0x401d48` |

(Dispatch values derived from the `cmp eax,8; je`(8) / `sub eax,0; je`(0) /
`dec;dec`(2) / `dec;dec`(4) / `dec`(5) chain, and the `>8` arm
`sub eax,9; je`(9) / `sub eax,6; je`(0x0F) / `dec`(0x10) at `0x402197`. `[CONFIRMED]`.)

**Character display:** cmd `0x04` runs a **7-segment translation** over 16 bytes
(`Smallcp.exe!0x401e64`: remaps `0x5e→0x5e, 0x5f→0x76, 0x60→0xb0, 0x7b/0x7f→0x3c,
0x7c→0x3e`, etc.) → the panel is an alphanumeric LED matrix (the "QuickType Mono"
font), not just discrete LEDs. cmd `0x08` moves a cursor (0–15), cmd `0x0F` sets a
blinking range — i.e. the master drives a full text UI on this panel.

### Outbound (panel → master) — `RemoteStatus(addr=[+0xba], out=[+0xaf], len=3, now=1)` `[CONFIRMED]`

Fixed **3-byte** frame `out = [+0xaf][+0xb0][+0xb1]` (sent from every command handler):

| byte | offset | value | cite |
|:----:|:------:|-------|------|
| `out[0]` | `+0xaf` | response cmd byte | (RemoteStatus dataPtr base) |
| `out[1]` | `+0xb0` | status bit — toggled to `0x80`/0 by inbound cmd `0x05` | `0x402099`–`0x4020ac` |
| `out[2]` | `+0xb1` | **button code (1–7), 0 = none** — loaded from `+0x195` on poll, then cleared | `0x402131`–`0x402160` |

**Button encoding** `[CONFIRMED]` — UI buttons queue a code into `+0x195` via
`SetButton(code) → [+0x195]=code` (`Smallcp.exe!0x402297`); the next `0x00` poll copies
it to `out[+0xb1]`. Constant-push wrappers at `0x4022a7`:

| button | code | wrapper |
|:------:|:----:|:-------:|
| 1 | `0x01` | `0x4022cf` |
| 2 | `0x02` | `0x4022c7` |
| 3 | `0x03` | `0x4022bf` |
| 4 | `0x04` | `0x4022b7` |
| 5 | `0x05` | `0x4022af` |
| 6 | `0x06` | `0x4022a7` |
| 7 | `0x07` | `0x4022d7` |

(7 function buttons — the "Fn Buttons" + "Delay Override" set named in the Setup menu.)
Unlike the all-button panels, the press is **latched** (`+0x195`) until the next poll,
not sent immediately.

---

## 4. Cross-panel summary

| panel | exe | class | base addr | addr method | LED/display inbound | buttons | outbound frame |
|-------|-----|:-----:|:---------:|-------------|---------------------|:-------:|----------------|
| Dual Spa switch | `2x4rem.exe` | `0x02` | **`0x10`** | manual probe (`RemoteAddressUsed`, seed `0x10`) | cmd `0x02`: 2 LEDs × 2-bit in `inBuf[1]` | 1–8 | `[0x01 0x00 btn]` |
| Simple AllButton | `8button.exe` | `0x04` | **`0x20`** | manual probe (seed `0x20`) | cmd `0x02`: ≥4 LEDs × 2-bit; cmd `0x03`: text | 1–9 | `[0x01 0x00 btn]` |
| Simple OneTouch | `Smallcp.exe` | `0x08` | **`0x40`** | `RemoteGetNextAddress(8)` | cmds `0x02/04/08/09/0F/10`: 16-char LED matrix + cursor + blink | 1–7 | `[respCmd b0 btn]` |

**Key takeaways for aqualink-automate**

* **New address families recovered** (extend `src/jandy/types/jandy_device_types.h`):
  * class `0x02` → **`0x10`–`0x17`** = 2x4 / dual-spa switch remote.
  * class `0x04` → **`0x20`–`0x27`** = "AllButton" 8-button keypad.
  * class `0x08` → `0x40`–`0x43` = OneTouch (confirms existing project mapping).
* **Panel→master button protocol** is uniform and simple: a fixed 3-byte status frame
  `[0x01][0x00][buttonCode]`, `buttonCode` 1..N, `0` when idle. For the simple panels
  the press is sent on the very next poll; the OneTouch latches it in an internal slot
  (`+0x195`) and emits it on the `0x00` poll. **This is the keypad/remote button-press
  wire encoding** — small integer button index, not a bitfield.
* **Master→panel LED protocol** uses **2 bits per indicator** (off/on/blink) packed into
  LED byte(s), carried under inbound **cmd `0x02`**. The OneTouch additionally has a full
  character-display vocabulary (cmd `0x04` text, `0x08` cursor, `0x0F` blink-range,
  `0x10` set-item) — useful if the project ever emulates a OneTouch *display sink*.
* These match the §2 DLE/STX base-Jandy frame; nothing here uses Pentair `0xA5`.

### Caveats / capture-gated `[INFERRED]`
* The simulators only *decode* the LED bytes they have UI controls for (2 for 2x4rem,
  ~4+ for 8button). The full byte→LED map for a real keypad with more indicators, and
  the exact meaning of the higher cached bytes, need a **live RS-485 capture** to pin
  down — do not over-encode beyond `inBuf[1]` from these binaries.
* The 3-byte outbound `out[0]`/`out[1]` constants (`0x01`/`0x00`) are what these sims
  seed; a real keypad's response-command byte for an *unsolicited* button event was not
  separately observed (the sims only ever answer a poll). Treat `0x01` as the
  sim-confirmed response cmd, button index as the load-bearing field.

### Provenance / re-derivation (per panel)
* **2x4rem:** addr probe `0x40115d`; logon `0x40132f`; GetMasterMessage `0x401535`
  (`inBuf=+0x7c`); RemoteStatus `0x40155d` (`out=+0xb0,len=3`); handler `0x40151f`;
  LED applier `0x401420`/`0x401454`; button setter `0x4015a4`, wrappers `0x4015b1`+.
* **8button:** addr probe `0x401156`; logon `0x4013cc`; GetMasterMessage `0x4016cd`
  (`inBuf=+0x80`); RemoteStatus `0x401707`/`0x40172d`/`0x40177e` (`out=+0xb4,len=3`);
  handler `0x4016b4`; LED applier `0x401483`; button setter `0x4013d9`, wrappers
  `0x4017be`+.
* **Smallcp:** `RemoteGetNextAddress(8)` `0x40194d`; GetMasterMessage `0x402045`
  (`inBuf=+0x9a`); RemoteStatus `0x4020be`+ (`out=+0xaf,len=3`); handler `0x40201f`;
  7-seg decode `0x401e64`; cursor `0x401cdb`; blink-range `0x401d48`; clear `0x401bd9`;
  button setter `0x402297`, wrappers `0x4022a7`+.
* Thunk verification (Smallcp): `0x402568`→`GetMasterMessage` IAT `0x4318c4`,
  `0x402562`→`RemoteStatus` IAT `0x4318c8`, `0x402544`→`RemoteGetNextAddress` IAT
  `0x4318dc`.
