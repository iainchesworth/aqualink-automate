# PDA / AquaPalm remote (`PDA.exe`) — protocol from the vendor's decoder

Static RE of the official Jandy *Aqualink RS Rev T.0.1* simulator
`C:\Program Files (x86)\Alwin32\PDA.exe` (1.4 MB debug MFC build, links `NetIO.dll`).
Read `docs/alwin32_simulator_protocol.md` first — this file extends §6's per-device recipe with the
**PDA / AquaPalm remote's** inbound/outbound message vocabulary.

Citations are `PDA.exe!0xRVA`. **Image base 0x400000**, so the toolkit RVA = address − 0x400000
(e.g. the inbound dispatcher at VA `0x4026e4` is `--rva 0x26e4`). Every claim below was read out of
the disassembly; items I could not pin to a concrete instruction are marked **INFERRED**.

> **Headline:** the PDA sim speaks the **exact same character-terminal opcode set and 12×16 display
> geometry as the OneTouch sim** (`findings/onetouch.md`) — `0x00/0x02/0x04/0x05/0x08/0x09/0x0F/0x10`,
> 12 lines × 17-byte stride. The ONLY protocol-level difference is the **RS-485 address class**: PDA's
> native class is **0x0C → `0x60`–`0x63`** (OneTouch is class 0x08 → `0x40`). The PDA sim can also be
> toggled (via a menu) to log on as class 0x08, i.e. masquerade as a OneTouch panel. Like OneTouch this
> is the **classic char-display protocol**, NOT the project's IAQ/AqualinkTouch page protocol
> (0x23/0x24/0x25/0x28/0x2d, 0x70, 0x72) — those live on the master / IAQ side and are out of scope here.

---

## 1. Identity  *(CONFIRMED)*

* Source project path baked into strings: `C:\AQUALINK\RS RevR\Alwin32 VC++ 5.0\PDA\PDA.cpp` /
  `PDADoc.cpp` / `PDAView.cpp` (`PDA.exe!0x128a70`, `0x128ae0`, `0x128b28`), classes `CPDADoc`
  (`0x128b14`), `CPDAView` (`0x128b64`).
* App chrome: `"PDA MFC Application"` (`0x141920`), `"PDA Version 1.0"` (`0x1413fa`),
  `"PDA Application"` (`0x141a74`), window title format `"PDA # %d"` (`0x128bb0`).
* Dual identity: it also carries `"OneTouch # %d"` (`0x128ba0`) and a **menu with both `&PDA`
  (`0x141278`) and `&OneTouch` (`0x141260`)** entries (plus `&ICON Bit`, `&Temperature Bit`,
  `&Delay Override`) — i.e. the same UI can run as either device. See §2.
* Imports from `NetIO.dll`: `RemoteLogOn, GetMasterMessage, RemoteStatus, RemoteLogOff, CommInUse,
  CommInit` (`info` import table). Note **no `RemoteGetNextAddress` import** — PDA composes its address
  byte itself from a class constant (§2), rather than negotiating an instance via NetIO.

---

## 2. Address — class **0x0C → `0x60`–`0x63`** (with a 0x08/0x40 "OneTouch" mode)  *(CONFIRMED)*

The address byte is assembled by a member method `PDA.exe!0x402aff`:
`addr = ((class & 0x1f) << 3) | (instance & 7)` (`0x402b29`–`0x402b58`), stored to the object at
`+0x244` (`0x402b4f`). This is exactly the reference doc's `(class<<3)|instance` (§3 of the shared doc).

That method takes **`class` as its first argument** (`[ebp+8]`, `0x402b26`) and `instance` as its
second (`[ebp+0xc]`, `0x402b39`). It has **two callers**, each a one-shot guarded toggle:

| caller | pushes | class | base addr | guard flag set | clears | cite |
|--------|--------|:-----:|:---------:|----------------|--------|------|
| `0x402c5c` | `push 0; push 0xC` | **0x0C** | **`0x60`–`0x63`** | `[obj+0x7c]=1` | `[obj+0x7e]=0` | `0x402c80`–`0x402c87` |
| `0x402c28` | `push 0; push 8`   | **0x08** | `0x40`–`0x43` | `[obj+0x7e]=1` | `[obj+0x7c]=0` | `0x402c4c`–`0x402c53` |

The two are mutually exclusive (each clears the other's flag), and they correspond to the **`&PDA` vs
`&OneTouch` menu items** (§1). So:

* **PDA native RS-485 address = class 0x0C = `0x60` (instance 0; `0x61`/`0x62`/`0x63` for 1–3).**
  Instance is hard-coded `0` here (`push 0`), so the sim always claims `0x60` unless changed.
* **Compatibility mode = class 0x08 = `0x40`** (poses as a classic OneTouch panel).

> **NEW vs project.** The project's notes have OneTouch at class 0x08 (0x40) and chlorinator at 0x0A
> (0x50) but **no entry for class 0x0C / 0x60 = the PDA/AquaPalm**. This is the new address fact.
> **INFERRED:** that real PDA hardware uses 0x60 (the sim hard-codes instance 0 → 0x60; on a live bus
> NetIO would normally negotiate the instance, but this exe sets it directly).

---

## 3. The device object (CPDADoc) — slot layout

The PDA keeps its state on its own MFC object (`this` in `ecx`, spilled to `[ebp-0x40]`/`[ebp-0x7c]`
across the dispatcher), mirroring the NetIO network-table slot. Offsets recovered from the dispatcher
(`0x4026a4`+), the text/cursor helpers, and LogOn (`0x402aff`):

| Offset | Size | Meaning | cite |
|-------:|-----:|---------|------|
| `+0x1c` | 4 | owner handle passed to `RemoteLogOn` | `0x402b85` |
| `+0x84` | 2 | "status dirty / ready" flag — copied into outbound byte `+0x22c` on poll, zeroed on reset | `0x40277f`, `0x4021d7` |
| `+0x88` | 2 | **currently-highlighted line index** (−1 = none) | `0x4028e9`, `0x402577`, `0x402507` |
| `+0x8a` | 2 | highlight attribute / msg1 (set by cmd 0x10) | `0x4025a6` |
| `+0x8e` | **12 × 0x11** | **the display: 12 line records, 0x11 (17) bytes each** (16 chars + NUL) | `0x4023e5`, `0x402a0a`, `0x402a1c` |
| `+0x210` | 2 | cursor/highlight row (set by cmd 0x10, cleared by cmd 0x08) | `0x40258b`, `0x4024f8` |
| `+0x212` | 2 | cursor/highlight col (cmd 0x10) | `0x402599` |
| `+0x214` | N | **inbound master-message buffer** — `[+0x214]=data[0]=cmd`, `[+0x215]=data[1]`, `[+0x216]=data[2…]` | `0x402715`, `0x40272a` |
| `+0x228` | 2 | length of last inbound message (`GetMasterMessage` return) | `0x4026ef` |
| `+0x22a` | 1 | **outbound response command byte** — = **`0x01`** | `0x4021b9` |
| `+0x22b` | 1 | outbound response flag byte 1 (bit 7 toggled by cmd 0x05) | `0x4021c0`, `0x4027ea` |
| `+0x22c` | 1 | outbound response flag byte 2 (= `+0x84` ready flag on poll) | `0x4021ca`, `0x402785` |
| `+0x244` | 1 | this device's **RS-485 address** (passed to every NetIO call) | `0x402b4f`, `0x40279c` |
| `+0x245` | 1 | instance (low 3 bits of address) | `0x402b5d` |

* **Display geometry CONFIRMED:** the row-repaint helper `0x402369` zeroes a 17-byte buffer
  (`cmp ecx,0x11`, `0x40238a`), clamps the line index `&0xf` then to **0..0xb (0–11)**
  (`0x4023ba cmp eax,0xb`), and copies it into `this + 0x8e + line*0x11` (`imul …,0x11`; `lea
  [ecx+eax+0x8e]`, `0x4023df`/`0x4023e5`). **12 lines × 16 visible chars (17-byte stride).**
* **LogOn** is `RemoteLogOn(handle=[+0x1c], addr=[+0x244], initBuf=&[+0x22a])` (`0x402b88`) — the init
  buffer is the same 3-byte outbound status block, i.e. the panel logs on advertising
  `[0x01][flag][flag]`.

---

## 4. Inbound message dispatch — master → PDA  *(the high-value part)*  *(CONFIRMED)*

The dispatcher calls `GetMasterMessage(addr=[+0x244], outBuf=&[+0x214])` (`PDA.exe!0x4026e4`, the only
call site), stores the returned length at `+0x228`, bails if it's 0 (`0x4026c5`), then dispatches on
**`data[0]` = `[+0x214]` = the message-type byte** via a jump table:

* range check `cmd ≤ 0x10` (`0x40275d cmp …,0x10; ja default`),
* byte index table at `PDA.exe!0x402a7f` (17 bytes, cmd 0x00..0x10),
* dword jump table at `PDA.exe!0x402a5b`.

Decoded exhaustively from those two tables (read directly out of `.rdata`):

| `data[0]` | handler RVA | name (mine) | what it does | cite |
|:---------:|:-----------:|-------------|--------------|------|
| **0x00** | `0x402779` | **Poll / keep-alive** | no display change; copies ready flag `[+0x84]`→outbound `[+0x22c]`, sends 3-byte status | `0x402779`–`0x4027a8` |
| **0x02** | `0x402779` | **Poll (alias of 0x00)** | *same handler as 0x00* — index table maps both cmd 0 and cmd 2 to the poll target | table @`0x402a7f`/`0x402a5b` |
| **0x04** | `0x402816` | **Set one display line** | sends ACK; sanitises glyphs in text (`0x4025c0`); writes line `data[1]` (0–11) ← text `data[2..17]` (16 chars) via `0x402369` | `0x402816`–`0x402854` |
| **0x05** | `0x4027c6` | **Toggle blink/flag** | sends ACK; flips bit 7 of outbound flag byte `+0x22b` (`[+0x22b] = ([+0x22b]&0x80)?0:0x80`) | `0x4027c6`–`0x402811` |
| **0x08** | `0x402859` | **Select / highlight line** | sends ACK; `0x4024c5(this, data[1])` — sets highlighted line `data[1]` clamped 0–15 (else −1) | `0x402859`–`0x402896` |
| **0x09** | `0x40292d` | **Refresh / new page** | sends ACK; `0x401140` (no-arg full repaint) | `0x40292d`–`0x402954` |
| **0x0F (15)** | `0x402959` | **Scroll / range line-copy** | sends ACK; block-moves a *range* of display rows (`data[1]`..`data[2]`, direction from sign of `data[3]`); each row copied src→dst (`imul 0x11`, `+0x8e`) then re-rendered (`0x4010cd`) | `0x402959`–`0x402a51` |
| **0x10 (16)** | `0x40289b` | **Set cursor / highlight pos** | sends ACK; if `data[4]!=2` and `data[1]` differs from current highlight `[+0x88]`, set cursor via `0x40100a(this, data[1], data[2], data[3])` (line, row, col) | `0x40289b`–`0x402928` |
| *(0x01,0x03,0x06,0x07,0x0a-0x0e)* | `0x402a53` | ignored (no-op `xor eax,eax; ret`) | | table |

**Recognised inbound type bytes: `0x00, 0x02, 0x04, 0x05, 0x08, 0x09, 0x0F, 0x10`** — identical set to
the OneTouch sim.

### Byte layouts (offsets relative to `data[0]` = type byte, i.e. into `+0x214`)

* **0x04 Set-line** — `[0x04][line:1][text: up to 16 bytes]`
  - `data[1]` = line index, clamped **0–11** (`0x4023ba`). `data[2..]` = the 16 chars of line text.
  - **Glyph sanitiser** `0x4025c0` runs over 16 chars (`cmp …,0x10`, `0x4025e1`); each byte `c`:
    compute `c-0x5e`; if in `0..0x21`, a small index table (`0x402679`) + jump table (`0x40265d`)
    maps these special control codes to a display glyph, else the char passes through unchanged.
    Concretely (table decoded):

    | wire byte | → display glyph | meaning (INFERRED from glyph) |
    |:--------:|:--------------:|------|
    | `0x5e` `^` | `0x5e` `^` | up-arrow |
    | `0x5f` `_` | `0x76` `v` | down-arrow |
    | `0x60` `` ` `` | `0xb0` | **degree sign °** |
    | `0x7b` `{` | `0x3c` `<` | left-arrow |
    | `0x7d` `}` | `0x3e` `>` | right-arrow |
    | `0x7e` `~` | `0x3e` `>` | right-arrow (alt) |
    | `0x7f` | `0x3c` `<` | left-arrow (alt) |

    So on the **wire**, text is ASCII with `^ _ \x60 { } ~ \x7f` reserved as arrow/degree glyphs; the
    sim only translates them for Windows-font rendering. (This is *richer than* the OneTouch sim's
    decode, which only mapped `^`/`_`→glyph 0x10 — see comparison §7.)
* **0x10 Set-cursor / highlight** — `[0x10][line:1][row:1][col:1][mode:1]`
  - `data[1]`=line, `data[2]`=row, `data[3]`=col (all read `movsx` = **signed**), `data[4]`=mode
    (`==2` suppresses the move, `0x4028da`). On apply: `[+0x88]=line`, `[+0x210]=row`,
    `[+0x212]=col`, `[+0x8a]=line` (only if no highlight is already active, `[+0x88]==−1`, `0x40257e`).
* **0x0F Scroll/range** — `[0x0F][a:1][b:1][dir:1]`
  - `data[3]` (`dir`) sign chooses copy direction (`+1`/`−1`, `0x4029a6`/`0x4029c6`); `data[1]`/`data[2]`
    are the inclusive row bounds; loop copies row→row in `+0x8e` (`imul 0x11`) and re-renders each.
* **0x08 Select/highlight line** — `[0x08][line:1]` — `line` clamped 0–15 else −1 (`0x4024ea cmp …,0xf`).
* **0x05 Toggle** — `[0x05]…` — flips bit 7 of outbound flag `+0x22b`; payload otherwise unused.
* **0x00 Poll / 0x02 Poll / 0x09 Refresh** carry no parsed payload.

> These are the **classic char-terminal opcodes**, distinct from the IAQ page messages the project
> models. **NEW vs project** (the project has no PDA opcode set documented).

---

## 5. Outbound — PDA → master  *(CONFIRMED for the wire frame; button codes NOT on the wire here)*

Every inbound handler that replies calls `RemoteStatus(addr=[+0x244], &[+0x22a], len=3, sendNow=1)` —
the panel's wire response is a **fixed 3-byte status frame `[+0x22a][+0x22b][+0x22c]`**
(`PDA.exe` thunk `0x4034c4`; **all 7** call sites at `0x4027a3, 0x402809, 0x40282e, 0x402871,
0x4028b3, 0x402944, 0x402970` — every one pushes `addr, &[+0x22a], 3, 1`).

* **Response command byte `[+0x22a]` is fixed `0x01`** (`0x4021b9 mov byte[+0x22a],1`; never rewritten
  in the dispatch). The master sees `data[0]==0x01` on every PDA→master status.
* **`[+0x22b]`** — flag byte; bit 7 toggled by inbound cmd 0x05 (`0x4027ea`).
* **`[+0x22c]`** — = the device "ready/dirty" flag `[+0x84]`, copied in on poll (`0x402785`).

**The PDA sim never spontaneously transmits and never sends a keypress/button code on the wire.** All 7
`RemoteStatus` calls are *responses inside the inbound dispatcher* (it is a pure poll-driven slave),
and the only writes to the outbound bytes `+0x22b`/`+0x22c` are the cmd-0x05 toggle and the cmd-0x00
ready-flag copy. There is **no outbound nav/button opcode** (no select/back/scroll/value-submit code)
on this binary's TX path.

> **CONTRADICTS the prompt's expectation of an outbound up/down/select/back encoding in this binary.**
> Like the OneTouch sim, the PDA panel here is a **remote display the master scripts**: the master
> *pushes* screen lines / cursor / scroll at it (§4), and the panel only answers with the fixed
> `[0x01][flag][flag]` status. The project's nav codes (0x01 select / 0x02 back / 0x06 scroll / 0x80
> submit) belong to the **IAQ/AqualinkTouch 0x33 command channel**, which this binary does not
> implement. (`0x01` *does* appear here, but as the **response command byte**, not a "select" opcode —
> do not conflate.) **INFERRED:** real PDA button presses are conveyed to the master either by the
> `+0x22b`/`+0x22c` flag bits (very low bandwidth) or by a mechanism the *master* (`Pwrcntr.exe`)
> drives — decode the master to recover the button-readback path.

---

## 6. Strings — no equipment/menu labels (dumb-terminal model)  *(supports §4)*

`strings PDA.exe --dedup` for POOL/SPA/HEAT/AUX/MENU/key/button → **no equipment or menu-item text**,
only MFC/CRT/Win32 boilerplate and app chrome (`PDA MFC Application`, `About PDA`, the `&PDA`/`&OneTouch`
mode menu). The display strings `"QuickType Mono"`/`"FarPoint Technologies"` (`0x1414a2`/`0x141b20`)
indicate a FarPoint grid control renders the LCD. This **confirms the panel holds no menu text of its
own — all 12 lines arrive over the wire** from the master via the `0x04`/`0x0F` opcodes (same as
OneTouch). No PDA button labels appear (the keys are bitmap/icon, drawn not labelled).

---

## 7. Compare to OneTouch (`findings/onetouch.md`)

| Aspect | OneTouch (`Onetouch.exe`) | PDA (`PDA.exe`) | verdict |
|--------|---------------------------|-----------------|---------|
| RS-485 class / base addr | 0x08 → **0x40**–0x43 | **0x0C → 0x60**–0x63 (or 0x08/0x40 in compat mode) | **DIFFERENT address, same scheme** |
| Address negotiation | `RemoteGetNextAddress(8)` (NetIO negotiates instance) | composes addr itself, instance hard-coded 0 (no `RemoteGetNextAddress` import) | different mechanism |
| Inbound opcode set | 0x00/0x02/0x04/0x05/0x08/0x09/0x0F/0x10 | **identical** 0x00/0x02/0x04/0x05/0x08/0x09/0x0F/0x10 | **SAME page protocol** |
| cmd 0x02 | **separate** status/icon handler (decodes `data[4]`/`data[5]` bitfields) | **alias of 0x00** (plain poll — index table sends both to one handler) | **DIFFERENT** (PDA has no icon-status handler) |
| Display geometry | 12 lines × 17-byte stride (16 chars) | **12 lines × 17-byte stride (16 chars)** | **SAME** |
| Outbound | fixed 3-byte `[0x01][flag][flag]` status, master scripts screen | **identical** fixed 3-byte `[0x01][flag][flag]` | **SAME** |
| Text glyph escapes | only `^`(0x5e)/`_`(0x5f) → glyph 0x10 | **richer**: `^`→`^`, `_`→`v`, `` ` ``→`°`, `{`/`\x7f`→`<`, `}`/`~`→`>` | **PDA decodes more glyphs** |

**Bottom line:** PDA and OneTouch sims run the **same classic char-terminal page protocol**; the PDA is
just that protocol on **address class 0x0C (0x60)**, with a slightly richer glyph set and `0x02`
folded into the poll. This is **NOT** the IAQ/AqualinkTouch page protocol (0x23/0x24/0x25/0x28/0x2d,
0x70, 0x72) the project currently models.

---

## 8. CONFIRMS / EXTENDS / CONTRADICTS vs project notes

| Project note | Verdict | Evidence |
|---|---|---|
| (no PDA address entry) | **NEW** | PDA = class 0x0C → `0x60`–`0x63` (`0x402c5c push 0xC`); compat mode 0x08/0x40 (`0x402c28`) |
| Master polls slave; slave sends 3-byte status | **CONFIRMS** | inbound `GetMasterMessage` `0x4026e4`; outbound `RemoteStatus(…,len 3, sendNow)` ×7 |
| Page messages 0x23/0x24/0x25/0x28/0x2d, 0x70/0x72, IAQ_Poll | **OUT OF SCOPE** | none of these type bytes appear in PDA's dispatch; it speaks the classic char-terminal opcodes (§4). Decode from `Pwrcntr.exe`/IAQ side. |
| Outbound nav codes 0x01 select / 0x02 back / 0x06 scroll / 0x80 submit | **NOT FOUND here** | PDA outbound is fixed 3-byte `[0x01][flag][flag]`; master scripts the screen. Those codes are the IAQ 0x33 channel. |
| Classic char-terminal opcodes 0x00/0x02/0x04/0x05/0x08/0x09/0x0F/0x10 on the PDA | **NEW** | jump tables `0x402a7f`/`0x402a5b`; layouts §4 |
| 12-line × 16-char display, 17-byte stride at `+0x8e` | **NEW/CONFIRMS** | repaint `0x402369` (clamp 0–11, `imul 0x11`, `+0x8e`) |
| PDA glyph escapes (`^ _ \x60 { } ~ \x7f`) | **NEW** | sanitiser `0x4025c0`, tables `0x402679`/`0x40265d` |

---

## 9. Net for the project + follow-ups

* **NEW address fact:** AquaPalm / PDA = **RS-485 class 0x0C, base `0x60`** (instances `0x60`–`0x63`),
  a class the project did not have. If aqualink-automate ever discovers/emulates a PDA, this is its id.
* The PDA runs the **same classic char-terminal page protocol as the classic OneTouch panel** (set-line
  `0x04`, scroll `0x0F`, set-cursor `0x10`, select `0x08`, clear/refresh `0x09`, toggle `0x05`, poll
  `0x00`/`0x02`) on a **12×16 character display** — concrete byte layouts in §4. **Not previously in
  the project.** Useful for interoperating with / decoding a *legacy* PDA remote.
* The **glyph escape map** (§4: `^`/`_`/`` ` ``/`{`/`}`/`~`/`\x7f` → arrows + `°`) is reusable for
  reconstructing PDA screen text from a capture.
* **Does NOT** yield the project's IAQ page-message field offsets, nor an outbound button-code table —
  the PDA panel is master-scripted. **Follow-up:** decode **`Pwrcntr.exe`** (the master that *emits*
  these line/cursor/scroll opcodes AND reads back button presses) to recover the button-readback path
  and the master-side field layouts the project actually wants.
