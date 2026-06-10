# iFlo.exe — Pentair IntelliFlo VS/VF pump (`_iFloVS` protocol family)

Reverse-engineered from the official Jandy *Alwin32* simulator suite
(`C:\Program Files (x86)\Alwin32\iFlo.exe` + `NetIO.dll`). Debug build
(`MFC71D.DLL`/`MSVCR71D.dll`, VS2003). PDB path embedded:
`c:\Projects\RevQ\Alwin32\iFloVS\Debug\iFloVS.pdb` (`iFlo.exe!0x18d64`).

All claims cited as `binary!RVA` (RVA = file/image RVA; image bases iFlo.exe `0x400000`,
NetIO.dll `0x10000000`, so VA = RVA for iFlo.exe). **CONFIRMED** = read directly from the
disassembly/data. **INFERRED** = deduced, not directly proven.

> Method note: `iFlo.exe` imports only **four** NetIO functions, all `_iFloVS`-suffixed:
> `RemoteStatus_iFloVS`, `GetMasterMessage_iFloVS`, `RemoteLogOn_iFloVS`,
> `RemoteLogOff_iFloVS` (`info iFlo.exe`). It does **not** import the base-Jandy NetIO API
> nor `RemoteGetNextAddress_iFloVS`. The `callsites` helper returned 0 for all of them
> because this PE has an empty `.textbss` section that *also* name-matches `.text`; the real
> `.text` is at RVA `0x11000`. Real callers were found by hand (E8 → ILT `E9` → import stub
> `FF25 jmp [IAT]`): each NetIO import is reached through an ILT thunk
> (`iFlo.exe!0x4117b2` GetMasterMessage, `0x4118b1` RemoteLogOn, `0x4112df`/`0x41161d`
> RemoteStatus/LogOff) and an import stub at `0x415fb8`–`0x415fca` (`FF25 jmp dword ptr [IAT
> 0x42bdc0..0x42bdcc]`).

---

## 1. Device identity & address — **CONFIRMED**

* UI / window title strings (`iFlo.exe` `.rsrc`/`.rdata`):
  `Pentair VF/VS Pump` (`0x42291e`, UTF-16), `Intelliflo VS` (`0x425418`),
  `Intelliflo VF` (`0x425408` / `0x415408`), labels `PUMP ADDR`, `WATTS`, `Priming Error`,
  `PUMP MODEL:`, `SPEED/FLOW`, `%d RPM` (`0x425428`), `%d GPM` (`0x425430`),
  `PUMP OFF`/`PUMP 1`..`PUMP 4` (`0x422e28`..). So it models a **Pentair IntelliFlo
  VS (variable-speed) or VF (variable-flow)** pump.

* **RS-485 address = `0x60`–`0x63`** (up to 4 pumps). The pump number (1–4, stored at
  object `+0xAC`) indexes a 5-byte table at `iFlo.exe!0x42a8c3` = `00 60 61 62 63`
  (`iFlo.exe!0x413bf2` `mov al,[ecx+0x42a8c3]` → object `+0x74`). Pump1→`0x60`,
  Pump2→`0x61`, Pump3→`0x62`, Pump4→`0x63`. Matches the project's
  `VSP/IntelliFlo pump (0x60–0x6F)`.

* The NetIO `_iFloVS` device table is **separate from the base-Jandy table**:
  base `0x100276c0`, **stride `0x99` (153) bytes, only 4 slots** indexed by `address & 3`
  (`NetIO.dll!RemoteLogOn_iFloVS@0x338c`: `imul ecx,ecx,0x99; add ecx,0x100276c0`;
  `FindNetTblIndex_iFloVS@0x3178` matches the address byte stored at slot `+0x98`,
  table at `0x10027758`). Contrast base Jandy: 16 slots × `0x36C`.

* `RemoteGetNextAddress_iFloVS` (`NetIO.dll!0x3541`) probes instances **0–3** and returns
  `base_addr + first_free_instance` (wraps to 0 if all four are in use). So the four pump
  addresses are consecutive `0x60..0x63`, exactly as the table above shows. (Note: the pump
  exe does **not** call this — the master controller sim does; the pump exe uses a fixed
  address from `+0x74`.)

---

## 2. Frame format on the wire — **the surprise**

### 2a. NetIO physical framing is **Jandy DLE, not Pentair `0xA5`** — CONFIRMED

`NetIO.dll` has exactly one transmit path, `CommTx` (`NetIO.dll!0x2ccb`), and it builds the
**base-Jandy DLE frame** for *all* protocol families including `_iFloVS`:

```
0x00 │ 0x10 0x02 │ dest │ data[0..] │ cksum │ 0x10 0x03 │ 0x00
pad    DLE  STX                       8-bit  DLE  ETX     pad
```

(`NetIO.dll!0x2d10` writes `0x10`, `0x2d1b` writes `0x02`; checksum seed `0x2d17`/`0x2d25`;
DLE-stuff of `0x10` in data). The RX state machine `CommRxNextChar` (`NetIO.dll!0x26bb`) is
likewise a `DLE(0x10)/STX(0x02)/ETX(0x03)` state machine. **There is no `0xA5` byte handling
and no 16-bit checksum anywhere in NetIO.dll's TX/RX.** `iFloNetIO`/`netSendMsg_iFloVS`/
`netGetMsg_iFloVS` (`NetIO.dll!0x3215`/`0x32e0`/`0x333b`) only manipulate the in-memory slot
table; they are never called internally (`CommTx` has a single caller, the base poll loop at
`NetIO.dll!0x1464`).

**Conclusion (CONFIRMED): the *simulator* carries the IntelliFlo protocol as the *payload*
of a Jandy DLE frame** — i.e. `RemoteStatus_iFloVS(addr, buf, len, …)` copies `buf` into the
slot and the master eventually ships it via the Jandy `CommTx`. The simulator is a training
emulator and reuses the one serial framer it has; it is **not** byte-for-byte the real
Pentair RS-485 line discipline.

### 2b. The IntelliFlo `0xFF 0x00 0xFF 0xA5` frame lives **inside the payload** — CONFIRMED

The application *payload* the pump hands to `RemoteStatus_iFloVS` for a status reply **is a
genuine Pentair frame**. In `GetMasterMessage_iFloVS`-driven handler (`iFlo.exe!0x41324e`,
the cmd `0x07` case), the 20-byte status buffer at object `+0x80` is built with the exact
Pentair preamble:

```
+0x80 = 0xFF   (iFlo.exe!0x413251)
+0x81 = 0x00   (0x41325b)
+0x82 = 0xFF   (0x413265)
+0x83 = 0xA5   (0x41326f)   <-- Pentair start-of-frame
```

So this confirms the project's `0xFF 0x00 0xFF 0xA5` preamble — **as the message content**,
which the real bus then frames. (The real Pentair bus uses `0xA5` + 16-bit checksum directly;
the simulator double-wraps it in DLE. For aqualink-automate the relevant part is the
**`0xA5` payload layout**, which is faithful.)

---

## 3. Inbound master→pump commands — CONFIRMED

`GetMasterMessage_iFloVS(addr, outBuf)` is called once at `iFlo.exe!0x4131f1`. The handler
(`iFlo.exe!0x4131b0`):
1. reads the message into `[ebp-0x1c]`, takes **`data[0]` = command byte** (`[ebp-0x25]`),
2. `cmd-1`, bounds-check `≤ 6` (`iFlo.exe!0x413234 cmp …,6 / ja`),
3. dispatches via jump table at **`iFlo.exe!0x413730`** (7 entries):

| `data[0]` | handler RVA | meaning (INFERRED from behaviour) | reply (CONFIRMED) |
|----------:|:-----------:|-----------------------------------|-------------------|
| `0x01` | `0x413526` | **Set speed / regulate.** Parses inbound; if `word[+0x7c]==0xC402` reads RPM `word[+0x7e]` (byte-swapped, §5), stores to global RPM `[0x42ab90]`, mode `[0x42ab8c]=0x4B`, sets `+0x75=2` (running), shows "Intelliflo VS". | `RemoteStatus(addr,&buf[+0x76],3,0)` (3-byte ACK, `0x413519`) |
| `0x02` | `0x4136d6` | no-op / default (shared exit) | none |
| `0x03` | `0x4136d6` | no-op / default | none |
| `0x04` | `0x4133d3` | **Pump control / mode.** Parses inbound; if `byte[+0x78]==4` → RPM`[0x42ab90]=0`, mode`[0x42ab8c]=0` (**STOP**). | `RemoteStatus(addr,&buf[+0x76],3,0)` (3-byte ACK, `0x4133e5`) |
| `0x05` | `0x413698` | **Local/remote-control mode.** Sets `+0x75=1`, shows "Intelliflo VF". | `RemoteStatus(addr,&buf[+0x76],3,0)` (3-byte ACK, `0x4136aa`) |
| `0x06` | `0x4133f2` | **Set RPM (display)** — same body as `0x04`; if `byte[+0x78]==4` stop, if `+0x75==2` formats "%d RPM" from `[0x42ab90]`. | `RemoteStatus(addr,&buf[+0x7a],3,0)` (3-byte ACK, `0x41368e`) |
| `0x07` | `0x41324e` | **Request status** — builds the full 20-byte `0xA5` status frame (§4). | `RemoteStatus(addr,&buf[+0x80],0x14=20,0)` (`0x4133c6`) |

Notes:
* The command IDs `0x01/0x04/0x05/0x06/0x07` line up with the real Pentair pump command set
  (0x01 regulate/speed, 0x04 run/mode, 0x05 local/remote toggle, 0x06 set RPM, 0x07 status
  request). The exact "set" payload layout is read through a parse helper
  (`iFlo.exe!0x41174e`, an ILT → copies the inbound bytes into the object at `+0x76`/`+0x7a`).
* `0xC402` (`iFlo.exe!0x41354b`) is a fixed two-byte signature the set-speed command must
  carry before the RPM word — **INFERRED** to be the Pentair "set speed register" selector
  (`0xC4 0x02` ≈ register/parameter id). Worth checking against a live capture.

---

## 4. Outbound status payload (cmd `0x07`) — byte-by-byte, CONFIRMED

Buffer = object `+0x80`, **length 20 (`0x14`)** (`iFlo.exe!0x4133b4 push 0x14`), sent via
`RemoteStatus_iFloVS(addr, &buf, 20, sendNow=0)` (`0x4133c6`).
Frame index = (obj offset − 0x80):

| idx | obj off | value | source / meaning | cite |
|----:|:-------:|-------|------------------|------|
| 0 | +0x80 | `0xFF` | preamble | `0x413251` |
| 1 | +0x81 | `0x00` | preamble | `0x41325b` |
| 2 | +0x82 | `0xFF` | preamble | `0x413265` |
| 3 | +0x83 | `0xA5` | Pentair SOF | `0x41326f` |
| 4 | +0x84 | `0x00` | version/sub byte | `0x413279` |
| 5 | +0x85 | *(unset here)* | **INFERRED** Pentair dest addr — left at logon-init default | — |
| 6 | +0x86 | *(unset here)* | **INFERRED** Pentair src addr — left at logon-init default | — |
| 7 | +0x87 | `0x07` | message type = 7 (status) | `0x41328d` |
| 8 | +0x88 | `0x0F` | **payload length = 15** | `0x413283` |
| 9 | +0x89 | `0x00` | reserved | `0x413297` |
| 10 | +0x8A | `0x00` | reserved | `0x4132a1` |
| 11 | +0x8B | `0x00` | reserved | `0x4132ab` |
| 12 | +0x8C | `hi(word[+0xA0])` | **running speed/RPM, high byte (big-endian)** | `0x4132b5`,`0x4132c2` |
| 13 | +0x8D | `lo(word[+0xA0])` | running speed/RPM, low byte | `0x4132cb`,`0x4132db` |
| 14 | +0x8E | `hi([0x42ab90])` | **commanded RPM, high byte (big-endian)** | `0x4132e1`,`0x4132ec` |
| 15 | +0x8F | `lo([0x42ab90])` | commanded RPM, low byte | `0x4132f2`,`0x4132ff` |
| 16 | +0x90 | `[0x42ab8c]` | **mode/status byte** (`0x4B` after set-speed, `0` when stopped) | `0x41331c`,`0x413322` |
| 17 | +0x91 | `0x00` | reserved | `0x413308` |
| 18 | +0x92 | `0x00` | reserved | `0x413312` |
| 19 | +0x93 | `0x08` / `0x00` | **run flag**: `0x08` when ramping/running (`+0xA4!=0 && +0x75==1`), else `0` | `0x413343`,`0x4133ab` |

* `word[+0xA0]` is the **live running speed** UI control (DDX control id `0x3EC`/`0x3E8`,
  `iFlo.exe!0x413052`/`0x413070`), shown as `%d RPM`/`%d GPM`. It is byte-swapped (§5) into
  bytes [12][13]. `[0x42ab90]` is the **commanded** RPM set by cmd `0x01`; bytes [14][15].
* Bytes [5][6] (`+0x85/+0x86`) are not written in any code path I found (no `mov …[+0x85]`
  anywhere in `.text`), so they carry whatever the `RemoteLogOn_iFloVS` 128-byte init buffer
  (`obj+0x94`) seeded — **INFERRED** to be the Pentair dest/src address pair. Flag for capture.

The two 3-byte ACK replies (cmds 0x01/0x04/0x05/0x06) send `&obj[+0x76]` or `&obj[+0x7a]`,
i.e. the raw 3 inbound parameter bytes echoed back — a short acknowledge, not a status frame.

---

## 5. Scaling / endianness — CONFIRMED

* **RPM is 16-bit big-endian** on the wire. Both the inbound set-speed value (cmd 0x01,
  `word[+0x7e]`) and the outbound status RPM fields are passed through the byte-swap helper
  `iFlo.exe!0x412df0` (reached via ILT `0x411a5a`): `sar ax,8` for the high byte, `shl ax,8`
  for the low byte, then `or` — a textbook `htons/ntohs`. The status builder also explicitly
  emits `sar …,8` then `and …,0xFF` (`0x4132bc`/`0x4132d2`/`0x4132e6`/`0x4132f7`) to split the
  16-bit values MSB-first into the frame. No further scaling (no imul/idiv) on the RPM — it
  is raw RPM.
* No GPM/Watts scaling math was observed in the status builder; "GPM"/"WATTS" appear as UI
  labels only — in this simulator the status frame carries **RPM (running + commanded) and a
  mode byte**, not watts/flow. (INFERRED: watts/flow are VF-model UI-only here; the real pump
  reports them in the longer status payload — capture-gated.)

---

## 6. Relevance to aqualink-automate `src/pentair/`

* **Confirms** the project's Pentair preamble `0xFF 0x00 0xFF 0xA5` (the simulator builds
  exactly this as the IntelliFlo status payload) and **IntelliFlo pump addresses `0x60–0x63`**
  (4 pumps).
* **Confirms 16-bit big-endian** field encoding for the pump (RPM hi-then-lo), matching the
  project note that Pentair uses a 16-bit big-endian checksum / big-endian fields.
* **Command vocabulary** for master→pump: `0x01` set-speed (with a `0xC4 0x02` register
  selector + BE RPM), `0x04` run/stop (stop = param byte `0x04`), `0x05` local/remote,
  `0x06` set-RPM, `0x07` request-status. These are candidate command IDs for
  `src/pentair/` pump control — useful even though the project's note says the *field offsets*
  are still capture-gated. The simulator's status frame fields (type=`0x07`, len=`0x0F`,
  RPM running, RPM commanded, mode byte, run flag) give a concrete starting layout to
  reconcile against a live IntelliFlo capture.
* **Caveat (important):** the simulator transports the `0xA5` payload inside a *Jandy DLE
  frame* (it has only one physical framer). So this RE confirms the **IntelliFlo
  application-layer payload** (preamble, type, length, RPM fields, mode/run bytes, BE
  encoding) but does **not** validate the real Pentair physical framing/16-bit checksum —
  that part of `src/pentair/pentair_checksum.h` must still be checked against real hardware
  or AqualinkD, not this sim.

---

## 7. Citations index

* `iFlo.exe` import set / PDB: `info iFlo.exe`; PDB string `iFlo.exe!0x18d64`.
* Address table: `iFlo.exe!0x42a8c3` = `00 60 61 62 63`; load at `iFlo.exe!0x413bf2`.
* RemoteLogOn call: `iFlo.exe!0x413c1b` (`handle=+0x20, addr=+0x74, init=+0x94`).
* GetMasterMessage handler/dispatch: `iFlo.exe!0x4131b0`, table `iFlo.exe!0x413730`.
* Command cases: `0x01`@`0x413526`, `0x04`@`0x4133d3`, `0x05`@`0x413698`,
  `0x06`@`0x4133f2`, `0x07`@`0x41324e`.
* Status frame bytes: `iFlo.exe!0x413251`–`0x4133b4` (table in §4).
* RemoteStatus calls: `iFlo.exe!0x4133c6`(len 20), `0x413519`/`0x4133e5`/`0x41368e`/`0x4136aa`(len 3).
* Byte-swap (BE): `iFlo.exe!0x412df0`.
* NetIO `_iFloVS` slot model: `NetIO.dll!RemoteLogOn_iFloVS@0x338c`, `RemoteStatus_iFloVS@0x345b`,
  `GetMasterMessage_iFloVS@0x349a`, `FindNetTblIndex_iFloVS@0x3178`,
  `RemoteGetNextAddress_iFloVS@0x3541`, `iFloNetRxBuf@0x30cd`; slot base `0x100276c0`, stride `0x99`.
* NetIO DLE framer (shared, no `0xA5`): `CommTx@0x2ccb`, `CommRxNextChar@0x26bb`.
