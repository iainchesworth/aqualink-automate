# OneTouch controller (`Onetouch.exe`) — protocol from the vendor's decoder

Static RE of the official Jandy *Aqualink RS* simulator `C:\Program Files (x86)\Alwin32\Onetouch.exe`
(debug MFC build, links `NetIO.dll`). Read `docs/alwin32_simulator_protocol.md` first — this file
extends §6's per-device recipe with the OneTouch panel's inbound/outbound message vocabulary.

Citations are `Onetouch.exe!0xRVA`. **Image base 0x400000**, so disasm RVAs are the address minus
0x400000 (e.g. the dispatcher at file/VA `0x401ca7` is `--rva 0x1ca7`). Every claim below was read
out of the disassembly; items I could not pin to a concrete instruction are marked **INFERRED**.

> **Scope note / big caveat.** This sim implements the **classic OneTouch panel protocol** (RS-485
> class 8 → `0x40`–`0x43`), which is the slave the master addresses with cmd `0x00` probes and feeds
> a 12-line × 16-char character display. **This is NOT the same wire vocabulary as the project's IAQ /
> AqualinkTouch page protocol** (MainStatus `0x70`, AuxStatus `0x72`, PageStart `0x23`, PageButton
> `0x24`, PageMessage `0x25`, PageEnd `0x28`, TitleMessage `0x2d`). The simulator's OneTouch panel
> message-type byte is a small enum (0,2,4,5,8,9,15,16) — see the table below — that drives a
> **character-cell display**, not the structured page messages. So this CONFIRMS the *address/class*
> and the *master-polls-slave / slave-sends-3-byte-status* model, but the rich page-message formats
> in the project come from the newer IAQ side and are **not** decodable from this binary. (`Pwrcntr.exe`,
> the master, is the place those would live — see follow-ups.)

---

## 1. Address — class 8 → `0x40`–`0x43`  *(CONFIRMED)*

`Onetouch.exe!0x4017a3`: `push 8` → `call RemoteGetNextAddress` (`Onetouch.exe!0x4017a5`, the one and
only call site). Class `8`, instance negotiated 0–3 by NetIO → bus addresses **`0x40` `0x41` `0x42`
`0x43`**.

**CONFIRMS** the project note and the reference doc (`Smallcp.exe!…GetNextAddress(8)`): OneTouch class
= 8, base 0x40.

---

## 2. The device slot (this exe's CWnd-derived device object)

Unlike NetIO's 0x36C network-table slot, the exe keeps its panel state on its own C++ object
(`ecx`/`esi` throughout the dispatcher). Offsets recovered from the dispatcher + LogOn init
(`Onetouch.exe!0x4017e4`, `0x401800`):

| Offset | Size | Meaning | cite |
|-------:|-----:|---------|------|
| `+0x1c` | 4 | window handle (HWND) used for all `SendMessageA`/`SetWindowText` | `0x4020ce` |
| `+0x7e` | N | **inbound master-message buffer** (filled by `GetMasterMessage`) — `[+0x7e]=data[0]=type`, `[+0x7f]=data[1]`, `[+0x80]=data[2]` … | `0x401cd1` |
| `+0x91` | 2 | length of last inbound message (`GetMasterMessage` return) | `0x401ce7` |
| `+0x93` | 1 | **outbound response command byte** — initialised to **`0x01`** at LogOn | `0x401800` |
| `+0x94` | 1 | outbound response byte 1 (init 0) | `0x401807` |
| `+0x95` | 1 | outbound response byte 2 (init 0) | `0x40180d` |
| `+0xaa` | 1 | this device's **RS-485 address** (passed to every NetIO call) | `0x401cb1` |
| `+0xfc` | 2 | current cursor/selected line index (−1 = none) | `0x401c17`,`0x401e6c` |
| `+0xfe` | 2 | cursor "dirty" flag | `0x401c40` |
| `+0x102` | 12×0x11 | **the display: 12 line records, 0x11 (17) bytes each** (16 chars + NUL) | `0x401b37`,`0x401ac6` |
| `+0x1ea` | 4 | HWND of the list-box that renders the 12 lines | `0x401b71` |
| `+0x27e` | 2 | cursor row | `0x401c72` |
| `+0x280` | 2 | cursor col | `0x401c7e` |

* **LogOn init buffer** = `{0x93:0x01, 0x94:0x00, 0x95:0x00}` (`Onetouch.exe!0x401800`–`0x401813`):
  the panel always logs on / answers the master with the **3-byte status `[0x01][0x00][0x00]`**.
* The string at `0x42c138` is `" "` (a space); the screen-clear helper `0x401ac4` fills all
  **12** line records with spaces (`push 0xc; …; add esi,0x11`) — **CONFIRMS 12 lines, 17-byte stride.**

---

## 3. Inbound message dispatch — master → OneTouch  *(the high-value part)*

The handler is `Onetouch.exe!0x401ca7` (`--rva 0x1ca7`). It only processes a message whose addressed
id `== [esi+0xaa]` (`0x401cba`) and non-zero length (`0x401cc3`), copies it into `+0x7e` via
`GetMasterMessage` (`0x401cd6`), then dispatches on **`data[0]` = `[+0x7e]` = the message-type byte**.

The dispatch is a value chain at `0x401d00`–`0x401d24` (low half) and `0x401e2e`–`0x401e3d` (high
half). Decoded exactly (cumulative `dec eax`):

| `data[0]` | handler RVA | name (mine) | what it does | cite |
|:---------:|:-----------:|-------------|--------------|------|
| **0x00** | `0x401dc8` | **Poll / Refresh** | no display change; copies `[+0x96]`→`[+0x95]`, then sends 3-byte status | `0x401dc8` |
| **0x02** | `0x401d91` | **Status/icon update** | reads flag bytes `data[4]`,`data[5]`; bit-tests (`data[4]&0x30`, `data[5]&0x03`, `data[4]&0xC0`, `data[5]&0x0C`) pick one of 3 mask values `+0xb2/+0xb6/+0xba`; `SetWindowText`s a status field. No ACK byte sent. | `0x401d91`–`0x401dc6` |
| **0x04** | `0x401d5c` | **Set one display line** | sends ACK; post-processes glyphs in `data[2..]`; writes line `data[1]` (0–11) ← text `data[2..]` | `0x401d5c`–`0x401d8c` |
| **0x05** | `0x401d2a` | **Toggle/flag** | sends ACK; `[+0x94] = ~[+0x94] & 0x80` (flips bit 7 of an outbound flag) | `0x401d2a`–`0x401d57` |
| **0x08** | `0x401e02` | **Select / highlight item** | sends ACK; selects list item `data[1]` (clamped 0–15) → `LB_SETCURSEL` | `0x401e02`–`0x401e29`, helper `0x401be7` |
| **0x09** | `0x401f21` | **Clear screen / new page** | sends ACK; clears all 12 lines + cursor (`0x401ae5`→`0x401ac4`) | `0x401f21`–`0x401f3d` |
| **0x0F (15)** | `0x401e92` | **Scroll / range line-copy** | sends ACK; block-moves a *range* of display lines (start `data[1]`, end `data[2]`, direction from sign of `data[3]`); each line copied src→dst record (`imul 0x11`) then re-rendered | `0x401e92`–`0x401f1f` |
| **0x10 (16)** | `0x401e43` | **Set cursor position** | sends ACK; if `data[4]!=2` and `data[1]` differs from current line, set cursor (row `data[2]`, col `data[3]`, line `data[1]`) via `0x401c54` | `0x401e43`–`0x401e8d` |
| *(other)* | `0x401f42` | ignored (no-op return) | | `0x401d24` |

**Recognised inbound type bytes: `0x00, 0x02, 0x04, 0x05, 0x08, 0x09, 0x0F, 0x10`.** (Confirmed by the
exhaustive `cmp/je` chain — no other value reaches a handler.)

### Byte layouts (relative to `data[0]` = the type byte)

All offsets are into the inbound buffer `+0x7e`, where `data[0]` is the type.

* **0x04 Set-line** — `[0x04][line:1][text…(≤16, NUL-terminated)]`
  - `data[1]` = line index, clamped to **0–11** (`0x401b23 cmp ax,0xb`).
  - `data[2..]` = ASCII line text, copied into a 17-byte record. **Glyph escapes:** chars
    **`0x5e` (`^`)** and **`0x5f` (`_`)** in the text are translated to glyph code `0x10`
    (`Onetouch.exe!0x402f11` — likely up/down-arrow or a custom LCD glyph). **INFERRED** which
    direction each maps to (only that both → 0x10).
* **0x10 Set-cursor** — `[0x10][line:1][row:1][col:1][mode:1]`
  - `data[1]` = line, `data[2]` = row coord, `data[3]` = col coord (both read `movsx`, i.e. **signed**),
    `data[4]` = mode (special-cased `==2`, `0x401e5d`).
* **0x0F Scroll/range** — `[0x0F][a:1][b:1][dir:1]`
  - `data[3]` (`dir`) sign chooses copy direction; `data[1]`/`data[2]` are the inclusive line bounds.
    Used to scroll the visible window of the 12-line buffer.
* **0x08 Select** — `[0x08][item:1]` — `item` clamped 0–15, drives `LB_SETCURSEL`.
* **0x02 Status** — `[0x02][?][?][?][flags4:1][flags5:1]…` — only `data[4]`/`data[5]` are decoded as
  bitfields (groups of 2 bits each). The three target strings are equipment/heater status labels.
  **INFERRED** exact meaning of each 2-bit group (sub/icon state) — the bit positions are concrete,
  the semantics are not labelled in this binary.
* **0x05 Toggle** — `[0x05]…` — flips bit 7 of the outbound flag byte `+0x94`; payload otherwise unused.
* **0x00 Poll** / **0x09 Clear** carry no parsed payload.

> These are the **classic OneTouch character-terminal opcodes**, distinct from IAQ page messages.
> They are **NOT in the project's notes** (the project models the IAQ 0x23/0x24/0x25/0x28/0x2d page
> protocol instead). If aqualink-automate ever needs to talk to a *classic* OneTouch panel (vs. emulate
> one toward the master), this is the opcode set. **NEW vs project.**

---

## 4. Outbound — OneTouch → master (button / status response)

Every inbound handler that produces a reply calls `RemoteStatus(addr=[+0xaa], &[+0x93], len=3,
sendNow=1)` — i.e. the panel's wire response is the **fixed-format 3-byte status frame
`[+0x93][+0x94][+0x95]`** (`Onetouch.exe!0x402fd8` thunk; 7 call sites at `0x401d4f, 0x401d6e,
0x401de6, 0x401e14, 0x401e55, 0x401ea4, 0x401f33`, all push `1,&[+0x93],3,addr`).

* **Response command byte `[+0x93]` is fixed `0x01`** (set once at LogOn `0x401800`, never rewritten in
  the dispatcher). So the master sees `data[0]==0x01` on every OneTouch→master status.
* **`[+0x94]` / `[+0x95]`** are the two status/flag data bytes. The dispatcher mutates them:
  - `0x05` flips `[+0x94]` bit 7 (`0x401d2a`: `cl=~[+0x94]; cl&=0x80; [+0x94]=cl`).
  - `0x00` poll copies `[+0x96]`→`[+0x95]` (`0x401dc8`) before answering.
  These look like **button-state / ack flags reflected back to the master**, not a rich keypress code.

**CONTRADICTS the prompt's expectation of explicit outbound nav codes (0x01 select / 0x02 back /
0x06 scroll / 0x80 value-submit) in THIS binary.** I could not find those constants on the outbound
path here. The classic-OneTouch sim's panel→master traffic is just the 3-byte `[0x01][flag][flag]`
status; the master *pushes* the screen at it. The select/back/scroll/value-submit codes the project
documents belong to the **IAQ / AqualinkTouch (0x33) command channel**, which this binary does not
implement. So: **NOT CONFIRMED, NOT CONTRADICTED for the IAQ codes — they're simply out of scope for
this binary.** (`0x01` does appear here, but as the *response command byte*, not a "select" opcode —
do not conflate.)

The cursor/select state the panel *does* track (`+0xfc` line, `+0x27e` row, `+0x280` col, `0x08`/`0x10`
handlers) is driven **by the master**, reinforcing that this panel is a remote display the master
scripts, not an autonomous menu walker.

---

## 5. Strings — nothing equipment-specific  *(supports the "dumb terminal" model)*

`strings Onetouch.exe --dedup` for POOL/SPA/HEAT/FILTER/PUMP/AUX/MENU/etc. → **no equipment or
menu-item labels** (only MFC/CRT/Win32 boilerplate, plus app chrome: `"ONETOUCH Windows
Application"`, `"OneTouch PC Version 5.0"`, `"Communication with Power Center has been established."`
`0x42c174`, `"Could NOT establish communication with Power Center." 0x42c13c`, `"COM PORT"/"SETUP"`).

This **confirms** the panel holds no menu text of its own: **all 12 lines of screen content arrive
over the wire** from the master (Power Center) via the `0x04`/`0x0F` line opcodes. The project's
screen-scraping model (reconstruct page text from wire bytes) is the right shape.

---

## 6. CONFIRMS / EXTENDS / CONTRADICTS vs project notes

| Project note | Verdict | Evidence |
|---|---|---|
| OneTouch = RS-485 address class 8 → 0x40–0x43, bus slave the master addresses | **CONFIRMS** | `push 8`→`RemoteGetNextAddress` `0x4017a5`; addr stored `+0xaa` |
| Master polls slave; slave sends status back | **CONFIRMS** | inbound via `GetMasterMessage`, outbound via `RemoteStatus(...,len 3, sendNow)` |
| Page messages PageStart 0x23 / PageButton 0x24 / PageMessage 0x25 / PageEnd 0x28 / TitleMessage 0x2d, MainStatus 0x70, AuxStatus 0x72, IAQ_Poll | **OUT OF SCOPE** (neither confirmed nor contradicted) | none of these type bytes appear in this binary's dispatch; it speaks the **classic** OneTouch char-terminal opcode set instead (§3). Decode these from `Pwrcntr.exe` / the IAQ side, not here. |
| Outbound nav codes 0x01 select / 0x02 back / 0x06 scroll / 0x80 value-submit | **NOT FOUND here** | this panel's outbound is a fixed 3-byte `[0x01][flag][flag]` status; master scripts the screen. Those codes live on the IAQ 0x33 channel. |
| 12-line × 16-char character display | **NEW/CONFIRMS** | 12 records × 0x11 bytes at `+0x102`; clamp 0–11; space-fill clear |
| Classic OneTouch opcodes 0x00/0x02/0x04/0x05/0x08/0x09/0x0F/0x10 | **NEW** | dispatch chain `0x401d00`/`0x401e2e`; layouts in §3 |
| Glyph escapes `^`(0x5e)/`_`(0x5f) → display glyph 0x10 in line text | **NEW** | `0x402f11` |

---

## 7. Net for the project

* Solidly **confirms address class 8 / 0x40–0x43** and the **poll-driven, slave-pushes-3-byte-status**
  model, plus the **12×16 character display** geometry.
* Surfaces the **classic OneTouch character-terminal opcode set** (`0x04` set-line, `0x0F` scroll,
  `0x10` set-cursor, `0x08` select, `0x09` clear, `0x02` status-icons, `0x05` toggle, `0x00` poll)
  with concrete byte layouts — **not previously in the project**. Useful if ever interoperating with a
  *legacy* OneTouch panel rather than the IAQ/AqualinkTouch path the project currently targets.
* **Does NOT** yield the IAQ page-message field offsets (0x23/0x24/0x25/0x28/0x2d, 0x70, 0x72) — this
  binary doesn't implement them. **Follow-up:** decode those from **`Pwrcntr.exe`** (the master that
  *emits* page/status messages) per the §6 recipe; that's where the title/button/line/page-type field
  layouts the project actually wants will be.
