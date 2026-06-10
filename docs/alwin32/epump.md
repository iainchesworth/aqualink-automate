# ePump.exe — Jandy ePump (Variable-Speed Pump) protocol

Reverse-engineered from the official Jandy *Alwin32* simulator
`C:\Program Files (x86)\Alwin32\ePump.exe` (debug MFC build, VS2003 / `mfc71d.dll`).
Method: static x86 disassembly (pefile + capstone). Every claim is cited as
`ePump.exe!0xRVA` (image base `0x400000`, so VA `0x41xxxx` == RVA `0x1xxxx`) or
`NetIO.dll!0xRVA`.

Cross-checked against the shared-protocol template
`docs/alwin32_simulator_protocol.md` and the chlorinator worked example.

**Legend:** `[CONFIRMED]` = directly observed in code/data; `[INFERRED]` = deduced
from data flow, flagged where the wire bytes themselves were not pinned down.

---

## 0. Identity / variants

* PDB path embedded in the binary: `c:\Projects\RevQ\Alwin32\jandyEpump\Debug\jandyEpump.pdb`
  — this is the **Jandy ePump** sim. `[CONFIRMED]` (`ePump.exe!0x17cf0`)
* UI resource strings: **"JANDY ePump"**, **"PUMP NUMBER:"**, **"RPM"**, **"WATTS"**,
  and `PUMP OFF`, `PUMP 1`, `PUMP 2`, `PUMP 3`, `PUMP 4`. `[CONFIRMED]`
  (`ePump.exe!0x2191e`, `0x219b4`, `0x219ec`, `0x21a14`, `0x21d78`–`0x21db6`)
* No fault / status / alarm / mode strings exist — the sim's modelled state is just
  **pump-number, RPM, and Watts**. The 6-byte status frame carries more bytes than the
  UI exposes (see §4).

### Which protocol family — *base Jandy*, NOT Pentair

`ePump.exe` imports only four NetIO symbols, all **base** (un-suffixed) variants:
`RemoteStatus`, `GetMasterMessage`, `RemoteLogOn`, `RemoteLogOff`
(`ePump.exe!info imports`). It does **not** import `RemoteGetNextAddress` (address is
hard-coded, §1), nor the `_ePumpAc` / `_iFloVS` variants.

Contrast: `iFlo.exe` (Pentair IntelliFlo sim) imports `RemoteStatus_iFloVS`,
`GetMasterMessage_iFloVS`, `RemoteLogOn_iFloVS`, `RemoteLogOff_iFloVS`
(`iFlo.exe!info imports`).

**Conclusion `[CONFIRMED]`:** the Jandy ePump rides the *standard Jandy* DLE/STX
RS-485 frame (`NetIO.dll!CommTx` §2 of the template), NOT the Pentair `0xA5` framing.
The `*_ePumpAc` NetIO exports exist but are **unused by this sim** — they are a separate
"ePump AC" mode and were not exercised here.

---

## 1. Addressing — class `0x0F`, addresses `0x78`–`0x7B` `[CONFIRMED]`

ePump does not negotiate an address; it indexes a static table by the user-selected
**pump number (1–4)**:

* `ePump.exe!0x4136db`: `al = byte[pumpNo + 0x4298bf]`; result becomes the device
  address at `obj+0x74`, passed to `RemoteLogOn` (§3).
* Address table at **`ePump.exe!0x4298bf`** (indexed by pump number; index 0 unused):

  | Pump # | Address byte | class (`addr>>3`) | instance (`addr&7`) |
  |-------:|:------------:|:-----------------:|:-------------------:|
  | (0 = OFF) | `0x00` | — | — (no log-on; guarded by `obj+0x90 > 0` at `0x4136c6`) |
  | 1 | **`0x78`** | `0x0F` | 0 |
  | 2 | **`0x79`** | `0x0F` | 1 |
  | 3 | **`0x7A`** | `0x0F` | 2 |
  | 4 | **`0x7B`** | `0x0F` | 3 |

  (Raw table bytes `00 78 79 7a 7b` at `0x4298bf`.)

* Independently confirmed: the re-logon handler hard-checks the current address against
  exactly `0x78`/`0x79`/`0x7A`/`0x7B` before tearing down/re-establishing
  (`ePump.exe!0x41367a`, `0x413686`, `0x413692`, `0x41369e`).

So the Jandy VSP **device family is class `0x0F` → bus addresses `0x78`–`0x7B`**,
i.e. up to 4 ePumps. This is consistent with the template's "`base, base+1, base+2,
base+3`" instance pattern and matches the well-known Jandy pump address block.

---

## 2. Log-on init buffer `[INFERRED — buffer contents not seeded in code]`

`RemoteLogOn(handle, addr, initBuf)` is called at `ePump.exe!0x4136fb` with:
* arg1 = `obj+0x20` (MFC handle)  `[CONFIRMED]`
* arg2 = `obj+0x74` (the `0x78`–`0x7B` address)  `[CONFIRMED]`
* arg3 = **`obj+0x82`** (pointer to the 128-byte init/status region)  `[CONFIRMED]`

The constructor (`ePump.exe!0x412d50`) zeroes `obj+0x8c/0x8e/0x90` and allocates a
0x80-byte buffer stored at `obj+0x88`, but does **not** seed `obj+0x82..` with constants,
so the initial 128-byte status payload is effectively zero-filled. `[INFERRED]`

---

## 3. The poll / dispatch loop (`GetMasterMessage`) `[CONFIRMED]`

`ePump.exe!0x41302d` calls `GetMasterMessage(addr, localBuf)` where `localBuf` is a
stack buffer at `[ebp-0x28]`; the returned **length** is stored at `obj+0x86`
(`0x413038`). If length == 0 it does nothing (`0x41304b`).

The **inbound command byte is `localBuf[0]`** (`mov al,[ebp-0x28]` at `0x413051`).
Dispatch: `cmd - 0x41`; `cmp 4; ja default`; `jmp [cmd*4 + 0x4132c8]`
(`ePump.exe!0x413067`–`0x413083`). Jump table at `0x4132c8`:

| cmd byte | ASCII | handler RVA |
|:--------:|:-----:|:-----------:|
| `0x41` | `'A'` | `0x4131e0` |
| `0x42` | `'B'` | `0x413210` |
| `0x43` | `'C'` | `0x413262` |
| `0x44` | `'D'` | `0x41313e` |
| `0x45` | `'E'` | `0x41308a` |

So the **master→pump command set is ASCII `'A'`–`'E'`** — a text-keyed protocol, unlike
the chlorinator's numeric `0x11`/`0x14`/`0x16`. Any command outside `0x41`–`0x45` is
ignored (default arm at `0x413283`).

### Inbound master commands

The handlers `memcpy` the raw inbound frame into the object so payload bytes land at
fixed offsets, then react. `obj+0x7c` is the **start of the 6-byte outbound payload**
(§4), so a `memcpy` to `obj+0x78` makes the *inbound* bytes 4,5 overlap the *outbound*
payload's data[0],data[1] (the sim echoes them).

| cmd | meaning (recovered) | code | payload handling |
|:---:|---------------------|------|------------------|
| `'A'` (0x41) | **Poll / status request** | `0x4131e0` | zeroes resp `+0x80/+0x81`; sends 6-byte status (§4). No state change. |
| `'B'` (0x42) | **Stop / pump off** | `0x413210` | zeroes resp `+0x80/+0x81`, sends status, **then zeroes RPM (`+0x8c`) and Watts (`+0x8e`)** (`0x413241`,`0x41324f`). |
| `'C'` (0x43) | **Start / run** (sets a mode byte) | `0x413262` | sets resp byte **`+0x7e = 0x0B`** (`0x413265`), sends status. |
| `'D'` (0x44) | **Set speed (RPM setpoint)** | `0x41313e` | `memcpy(obj+0x78, inbuf, len)`; reads a 16-bit setpoint from the frame; updates displayed RPM (see below). |
| `'E'` (0x45) | **Query value** (selector in frame) | `0x41308a` | `memcpy(obj+0x75, inbuf, len)`; returns RPM or Watts per a selector byte (see below). |

#### `'D'` — Set speed `[CONFIRMED arithmetic]`
After `memcpy(obj+0x78, inbuf, len)` (`0x413154`): inbound `msg[2]` → `obj+0x7a`,
`msg[3]` → `obj+0x7b`.
* `value = (msg[3] << 8) | msg[2]` — a **16-bit speed setpoint, low byte first on the
  wire** (`0x41315f`–`0x413183`, stored to global `0x429b8c`).
* Displayed RPM = `value >> 2` (`sar eax,2` at `0x4131bc`) → stored to `obj+0x8c`.

So **the wire speed value is RPM × 4** (quarter-RPM units). Example: 3000 RPM ⇒ wire
value `0x2EE0` ⇒ `msg[2]=0xE0`, `msg[3]=0x2E`. The raw ×4 value is cached in global
`ePump.exe!0x429b8c` and is what `'E'` query-0 reads back (below). `[CONFIRMED]`

#### `'E'` — Query value `[CONFIRMED]`
After `memcpy(obj+0x75, inbuf, len)` (`0x4130a0`): inbound `msg[2]` → `obj+0x77` acts as
a **selector**:
* selector `== 0` → return the cached speed value (global `0x429b8c`):
  `resp+0x80 = value & 0xFF`, `resp+0x81 = value >> 8`
  (`0x4130b3`–`0x4130d1`). I.e. **16-bit speed (×4), low byte at +0x80**.
* selector `== 5` → return **Watts** (`obj+0x8e`): `resp+0x80 = watts & 0xFF`,
  `resp+0x81 = watts >> 8` (`0x4130ef`–`0x413119`). **16-bit Watts, low byte at +0x80.**
* any other selector → no value written (status still sent).

Selectors 1–4 are recognised by the range check but produce no value in this sim
(only 0 and 5 are handled) `[CONFIRMED]`. Their real meaning (likely GPM/flow/temp on
real hardware) is **not determinable from this binary** — do not assume.

---

## 4. Outbound response (`RemoteStatus`) — **6-byte frame** `[CONFIRMED length/offsets]`

Every command handler ends by sending the device→master status the same way
(`ePump.exe!0x411622` thunk → `NetIO.dll!RemoteStatus 0x1fb5`), with identical args:

```
RemoteStatus(addr = obj+0x74,           ; the 0x78..0x7B pump address
             dataPtr = obj+0x7c,        ; payload buffer
             dataLen = 6,               ; always 6 bytes
             sendNow = 1)               ; transmit immediately
```
(push order at e.g. `0x4131a1`–`0x4131af`: `push 1; push 6; push (obj+0x7c); push addr`.)

So the **status payload is the 6 contiguous bytes `obj+0x7c .. obj+0x81`**:

| payload byte | struct off | source / meaning | confidence |
|:------------:|:----------:|------------------|:----------:|
| data[0] | `+0x7c` | **response command byte** — never written by the sim; = echo of inbound `msg[4]` (memcpy overlap) or stale | `[INFERRED]` |
| data[1] | `+0x7d` | echo of inbound `msg[5]` / stale | `[INFERRED]` |
| data[2] | `+0x7e` | mode/run byte — set to **`0x0B`** by `'C'` (start); otherwise stale | `[CONFIRMED for 'C']` |
| data[3] | `+0x7f` | echo of inbound `msg[7]` / stale | `[INFERRED]` |
| data[4] | `+0x80` | **value low byte** — RPM(×4) or Watts low, set by `'E'`; zeroed by `'A'/'B'/'D'` | `[CONFIRMED]` |
| data[5] | `+0x81` | **value high byte** — RPM(×4) or Watts high, set by `'E'`; zeroed by `'A'/'B'/'D'` | `[CONFIRMED]` |

**Important honesty note:** this sim **does not synthesise a clean, self-describing
status frame**. `obj+0x7c..0x7f` are never explicitly populated with constants — they
are leftovers from the `memcpy` of the *inbound* command (the ePump effectively *echoes*
the master's frame back, then overlays the requested value into bytes 4–5). The
*meaningful* outbound content the sim controls is:
* `+0x7e = 0x0B` after a `'C'` (start) command, and
* `+0x80/+0x81` = the little-endian 16-bit value selected by the preceding `'E'` query
  (RPM×4 for selector 0, Watts for selector 5).

The exact data[0] response-command byte and the framing of a real pump's *unsolicited*
status are **NOT** pinned down by this binary and must be confirmed against a live
capture before trusting a byte layout. Do not fabricate one.

### Internal state offsets (for reference)
| offset | meaning |
|:------:|---------|
| `+0x20` | MFC window handle (RemoteLogOn arg1) |
| `+0x74` | RS-485 address (`0x78`–`0x7B`) |
| `+0x75..` | inbound `'E'` frame copy (`+0x77` = `'E'` selector) |
| `+0x78..` | inbound `'D'` frame copy (`+0x7a/+0x7b` = speed setpoint low/high) |
| `+0x7c..+0x81` | **outbound 6-byte status payload** |
| `+0x82` | RemoteLogOn init-buffer pointer base |
| `+0x86` | last inbound message length (from `GetMasterMessage`) |
| `+0x88` | ptr to 0x80-byte allocated buffer |
| `+0x8c` | **displayed RPM** (= speed value ÷ 4) |
| `+0x8e` | **Watts** |
| `+0x90` | pump number (1–4; 0 = OFF) |
| global `0x429b8c` | cached 16-bit speed value (RPM × 4) |

---

## 5. Summary table — Jandy ePump VSP protocol

* **Family:** standard Jandy DLE/STX frame (`NetIO.dll!CommTx`); **NOT** Pentair `0xA5`.
* **Address class `0x0F` → `0x78`–`0x7B`** (4 pumps). `[CONFIRMED]`
* **Master→pump commands** (`data[0]`, ASCII):

  | cmd | meaning | key payload |
  |:---:|---------|-------------|
  | `'A'` 0x41 | poll / status request | — |
  | `'B'` 0x42 | stop (clears RPM+Watts) | — |
  | `'C'` 0x43 | start / run | sets resp `+0x7e=0x0B` |
  | `'D'` 0x44 | set speed | `msg[2..3]` = RPM×4, **low byte first** |
  | `'E'` 0x45 | query value | `msg[2]` selector: 0=speed(×4), 5=Watts |

* **Pump→master status:** fixed **6-byte** payload, sent immediately on every command;
  the sim-controlled fields are `+0x7e` (mode, `0x0B` after start) and `+0x80/+0x81`
  (little-endian 16-bit value = RPM×4 or Watts per the last `'E'` selector). Leading
  bytes are echoes of the inbound frame — a real pump's status framing is unconfirmed
  here.

---

## 6. Bearing on the aqualink-automate project

* **New address knowledge:** Jandy VSP = class `0x0F`, addresses **`0x78`–`0x7B`**.
  Worth adding to the device-id map (`src/jandy/types/jandy_device_types.h`) if not
  already present, distinct from the Pentair IntelliFlo path (`src/pentair/`).
* **The Jandy ePump speaks the *base* Jandy frame**, not Pentair `0xA5` — so a Jandy VSP
  decoder belongs under `src/jandy/`, reusing the existing DLE/STX deserialiser, with
  ASCII command bytes `'A'`–`'E'`.
* **Speed scaling:** wire speed = **RPM × 4** (quarter-RPM units), 16-bit **little-endian
  on the wire** (low byte first). Watts are a plain 16-bit value. Both are returned via
  the `'E'` query (selector 0 = speed, 5 = watts).
* **Caveat / capture-gated:** the simulator's outbound status frame is largely an echo of
  the inbound command — it does not reveal the real pump's unsolicited status layout
  (the data[0] response command byte, GPM/flow, fault bits). Treat the §4 byte map as
  *confirmed for offsets `+0x7e`/`+0x80`/`+0x81` only*; everything else needs a live
  RS-485 capture from a real Jandy ePump before being encoded as a message type. This
  is the analogue of the chlorinator's "trailing 16-bit field — worth identifying on a
  capture" caveat.

---

### Provenance / re-derivation
* Address table: `ePump.exe!0x4298bf` (raw `00 78 79 7a 7b`).
* Dispatch: `ePump.exe!0x413051`–`0x413083`; jump table `0x4132c8`.
* Handlers: `'A'`=`0x4131e0`, `'B'`=`0x413210`, `'C'`=`0x413262`, `'D'`=`0x41313e`,
  `'E'`=`0x41308a`.
* RemoteStatus call sites (all len=6, ptr=`obj+0x7c`): `0x413131`, `0x4131af`,
  `0x413206`, `0x413236`, `0x41327b`.
* RemoteLogOn: `0x4136fb`; RemoteLogOff: `0x4136aa`/`0x412c24`; GetMasterMessage:
  `0x41302d`. (Call sites resolved through the MSVC debug double-thunk chain
  `E8 → ILT(E9) → IAT(FF25)`; the auto-callsites tool only follows the single FF25 hop,
  so these were found manually.)
* NetIO `RemoteStatus` export = `0x1fb5`, `GetMasterMessage` = `0x2068`,
  `RemoteLogOn` = `0x1dff` (base variants).
