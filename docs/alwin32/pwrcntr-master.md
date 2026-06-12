# `Pwrcntr.exe` — the RS PowerCenter **bus master**

Reverse-engineered from the official Jandy *Alwin32* simulator suite
(`C:\Program Files (x86)\Alwin32`, debug MFC builds, VS2003 / `mfc71d.dll`).
Method: static x86 disassembly (pefile + capstone, via `alw.py` + helper scripts
`findings/netio_decode.py`, `findings/netio_local.py`, `findings/callers.py`).

`Pwrcntr.exe` is the **master** (image base `0x400000`, so VA `0x4xxxxx` == RVA
`0xxxxxx`). Unlike the equipment sims (remote slaves that `RemoteLogOn` and answer
`RemoteStatus`), Pwrcntr **drives the bus**: it builds every outbound command, hands
it to `NetIO.dll`, and `NetIO` performs the send-and-wait-for-reply transaction.

Cross-checked against the shared-protocol template
`docs/alwin32_simulator_protocol.md` (§2 frame `CommTx`, §3 addressing, §4 the
16-slot network table). **Legend:** `[CONFIRMED]` = directly observed in code/data;
`[INFERRED]` = deduced from data flow.

---

## 0. Which NetIO functions the master uses `[CONFIRMED]`

`Pwrcntr.exe!info` import of `NetIO.dll`:
`netIO, iFloNetIO, ClearPollCount, netEnd, CommPort, CommInit, netInit,
PwrCenterSim, IncPollCount, PollsDone, CountStartTm, CommEnd, NetRxBuf,
iFloNetRxBuf`. It also imports `iodll.dll` (its own on-board hardware: relays,
LEDs, sensors, option DIP switches) — the master simulates the physical controller
hardware on one side and the RS-485 bus on the other.

Key NetIO export RVAs (from `NetIO.dll!info` + disasm):

| NetIO export | RVA | role |
|--------------|-----|------|
| `netIO` | `0x12b8` | **the master TX-and-wait primitive** (see §1). *Aliased entry of the big routine the disasm of `PwrCenterSim --name` lands in.* |
| `PwrCenterSim` | `0x1170` | tiny accessor — returns the "sim is running" flag `byte[0x10027b90]` (used by the Start routine to detect "ALREADY RUNNING"). NOT the poll loop. |
| `CommTx` | `0x2ccb` | builds the DLE/STX framed frame + checksum (template §2). |
| `netSendMsg` | `0x1c60` | copies a command into a device slot's inbound buffer (`slot+0xB0`), len `0x2bc`, clears the slot dirty flag `slot+0x84`. |
| `iFloNetIO` | `0x3215` | Pentair IntelliFlo VS path (0xA5 family); `netSendMsg_iFloVS 0x32e0`, `CommTx`-equiv inside. |
| `ePumpAcNetIO` | `0x36ea` | Jandy ePump AC path; `netSendMsg_ePumpAc 0x37b3`. |
| `IncPollCount`/`PollsDone`/`ClearPollCount`/`CountStartTm` | `0x…` | poll-cycle counters / timing (poll-rate accounting). |

---

## 1. `netIO(msg)` — the master send/receive primitive `[CONFIRMED]`

`netIO` (`NetIO.dll!0x12b8`) is what *every* Pwrcntr command-builder calls. Its
single argument is a **message struct**:

```
struct NetMsg {
  +0x00 : u8   dest        // RS-485 destination; class = (dest>>3)&0x1f, inst = dest&7
  +0x01 : u16  length      // number of data bytes
  +0x03 : u8   data[0]     // COMMAND byte
  +0x04 : u8   data[1..]   // payload
}
```

Decoded behaviour (`NetIO.dll!0x12b8`):
* `0x12f3` → looks up the device slot for `data[0]`-ish key; `0x13ca` reads
  `data[0]`, increments per-slot stat counter at `slot+0x85` (TX-attempt count).
* `0x13fa`–`0x1405`: **`class = (dest>>3) & 0x1f`**; if `class==6` *and* `data[3]!=0`,
  inter-frame guard time = `0x1c2` (450) else by `data[3]`: `0x12c`(300)/`0x32`(50);
  reply-window = `0xc8`(200)/`0x1c2`. **Class 6 = addr `0x30`–`0x33` (AqualinkTouch /
  iAQ) gets the longest 450 ms timing** — matches the project's note that 0x33 rich
  status is slow. `[CONFIRMED]`
* `0x1452`–`0x1464`: `CommTx(dest=byte[msg], data=msg+3, len=word[msg+1])` — the
  actual wire send (template §2 framing).
* `0x1473`: `delay(2, guard)`; then waits/listens; on a reply it writes the
  **responder's address + payload back into `msg`** (the `dest` byte at `msg+0` is
  overwritten by the responder's id, and `word[msg+1]` by its reply length — see
  `0x42bba3` `mov ax, word[edx+1]` reading the reply length back out).
* On success the function returns `1` (`al`), and the caller reads `byte[msg+0]`
  (now the responder address) to learn **who answered**.
* Per-slot counters bumped on the reply path: `+0x89` (rx ok), `+0x8d`/`+0x91`
  (timeout/error variants), `+0x95/+0x99/+0x9d/+0xa1/+0xa9` (a stats block).

**Consequence — the master's poll/transaction pattern is a tight loop:**

```
build msg (dest=0xFF, data[0]=cmd, payload…)
for instance = 0 .. 4:        //  cmp 5 everywhere
    byte[msg+0] = 0xFF        //  broadcast / "anyone of this command's class"
    if netIO(msg) == 1:       //  someone answered
        break
if answered:
    respClass = byte[msg+0]   //  responder wrote its address here
    if respClass == <expected>:  consume reply payload
```

The **`for inst 0..4; dest=0xFF; netIO; break-on-reply`** idiom appears verbatim in
every builder (`0x401feb`@GetId, `0x403d6d`@heater, `0x46f019`@iAQ-page, …). It IS
the master's per-command poll-with-retry. `[CONFIRMED]`

The single global TX message buffer is **`0x5587c8`** (resource-named "A"); a cached
pointer to its `dest` field is **`0x557df0`**, and **`0x557df4`** ("T") is the
controller-state/config object the builders read setpoints/flags from. A second
buffer **`0x553bcc`** is used for ChemLink. `[CONFIRMED]`

---

## 2. Master → device command vocabulary (base `netIO`) `[CONFIRMED bytes]`

Recovered by decoding all **62 `netIO` call sites** (`netio_decode.py` +
`netio_local.py`) — cmd byte (`data[0]`), length, immediate payload, and where
available the responder-class compared after the reply. Citations are the call-site
VA in `Pwrcntr.exe`.

### 2a. Generic bus poll / keep-alive
| cmd | len | builder | meaning |
|----:|----:|---------|---------|
| **`0x00`** | 1 | `0x421f73` → `0x421fa9` | **bare poll / keep-alive** (`data={0x00}`, dest looped 0xFF). The universal device poll — matches the project's `0x00` poll for chlorinator/heater/aux/chem. |

### 2b. Device login / identity
| cmd | len | builder | payload | notes |
|----:|----:|---------|---------|-------|
| **`0x14`** | 2 | `0x401f9a`→`0x402011` | `data1=0x01` | **GetId / login probe.** Loops dest 0xFF inst 0..4; on reply checks responder, then copies the model-id string into `0x559340`. Confirms the chlorinator `0x14 GetId` and is the master's general identity query. |
| **`0x05`** | 1 | `0x447e20` | — | identity/version variant (short query). |

### 2c. Heater (class `0x0d` → addr `0x68`–`0x6b` LXi / `0x70` HeatPump / `0x38` LX) `[CONFIRMED]`
| cmd | len | builder | payload | responder-class check |
|----:|----:|---------|---------|-----------------------|
| **`0x0C`** | 5 | `0x403cd5`→`0x403d8a`, also `0x4157ef` | `data={0x0C,0,0,setpoint,..}` (`payload[2]=setpoint`, e.g. `0x0A`) | **`cmp eax,0x0d`** @`0x415818` / `0x403db6` — heater. **Confirms project's `0x0C enable+setpoint` for HeatPump/LXi/LX.** |

### 2d. iAqualink / AqualinkTouch (class 6 = `0x30`–`0x33`) — the WiFi/page side `[CONFIRMED]`
Gated by the **iAqualink-present flag `byte[0x5587d0]` ("i")** — these only run when
an iAqualink module is configured. The master *drives the page UI protocol* exactly
as documented in the project's `iaq_protocol.md`:

| cmd | len | builder | payload | meaning (project name) |
|----:|----:|---------|---------|------------------------|
| **`0x1c`** | 2 | `0x402c41`→`0x402c99` | `data1=0x00` | iAQ control/select (responder `cmp eax,0x1d` @`0x402cc5`). |
| **`0x1e`** | 2 | `0x401c40` | `data1=<arg>` | iAQ command (dest `0x98`); called from 3 sites (`0x42bf19`,`0x430a4c`,`0x472166`). |
| **`0x23`** | 2 | `0x46f008`→`0x46f03f`, `0x475bfe`, `0x475ee5`, `0x479bb3` | `data1=0x2a` or `0x5b` | **PageStart / PageButton.** `payload 0x2a` = **page type EquipmentStatus** (matches project's `0x2a=EquipmentStatus`); `0x5b` another page. |
| **`0x24`** | var | `0x455570` | — | **PageButton.** |
| **`0x26`** | var | `0x455754`, `0x479acd` | — | **PageMessage.** |
| **`0x27`** | 6 | `0x4557db` | 5 payload bytes | page sub-message. |
| **`0x28`** | 6 | `0x4574ee` | 5 payload | **PageEnd.** |
| **`0x31`** | 2 | `0x455a1c` | — | iAQ ControlReady / value (matches project `0x31 IAQ_ControlReady`). |
| **`0x41`** | 2 | `0x45594a` | — | iAQ field. |
| **`0x42`** | 5 | `0x4559b7` | 3 payload | iAQ field. |

The same big iAQ block (`0x455459`–`0x455f1d`) builds cmds `0x24/0x26/0x27/0x28/
0x31/0x41/0x42` with variable lengths — this is the master's **PageStart/PageButton/
PageMessage/PageEnd page-walk engine** for the AqualinkTouch (0x33) / iAQ (0xA3)
device, confirming the project's reverse-engineered iAQ page protocol *from the
vendor's master side*.

#### iAqualink WiFi-module command name table `[CONFIRMED]`
Inside `netIO`'s debug logger (`NetIO.dll!0x100016ba`) a jump table
(`bytemap@0x10001991`, `jt@0x10001951`) maps `data[0]` → a printable command name —
the **iAqualink2 module command set**:

| data[0] | name | | data[0] | name |
|--------:|------|-|--------:|------|
| `0x00` | `ENQ` | | `0x59` | `CALC RSSI` |
| `0x53` | `SSID` | | `0x5A` | `PERIODIC LOGGING` |
| `0x54` | `NETKEY` | | `0x5B` | `EVENT LOGGING` |
| `0x55` | `ENCRYPT` | | `0x5C` | `REGION` |
| `0x56` | `NETLINK STATUS` | | `0x70` | `MAINSCR_INFO` |
| `0x57` | `GET RSSI` | | `0x71` | `ONETOUCH_INFO` |
| `0x58` | `REV` | | `0x72` | `AUXILIARY_INF` |
|        |      | | `0x73` | `IPHONE_INFO` |

(Cited `NetIO.dll!0x100016f7`–`0x10001781` string loads.) These are the
**iAqualink (0xA0-0xA3) application commands** the master logs on TX — directly
useful for the project's iAQ work. `[CONFIRMED]`

### 2e. Aux / Remaux / panel-side (class 2 = `0x10`/`0x20`) `[CONFIRMED bytes]`
| cmd | len | builder | meaning |
|----:|----:|---------|---------|
| **`0x13`** | 0x18 (24) | `0x42a5de`→`0x42a64c` (payload filled by `0x42984f`) | **bulk controller-status broadcast** (24-byte status the master pushes out, e.g. relay/LED/temperature snapshot). |
| **`0x13`** | 5 | `0x42a687`→`0x42a6e8` | short status variant. |
| **`0x03`** | 0x12 (18) | `0x47c6b1` | **LED/7-seg display state push** to a panel (`payload[0]=0x00`, 18 bytes) — matches template's "Panels `0x04` 7-seg / `0x02` LED-state" family (here a combined 18-byte block). |
| **`0x08`** | 4 | `0x47c73b` | **set-relay** (`payload[0]=0x00`,+2 var) — matches project's Remaux `0x08/0x09 set relay`. |
| **`0x09`** | 3 | `0x415ffd` (buf `0x553bcc`) | **ChemLink/relay set** (`payload[1]=0x02`) — matches project's ChemLink/aux `0x09`. |
| **`0x07`** | 2 | `0x42bb7d` | status read (returns `word` reply into `[ebp+0xc]`). |

### 2f. Local-buffer iAQ builders `[CONFIRMED]`
Some iAQ/WiFi commands build the message on the **stack** (not the global "A"
buffer): `lea [ebp-0x84]` = `msg`, data region `[ebp-0x81]` = `msg+3`. Layout
identical (`+0` dest, `+1` word len, `+3` cmd). Example
`Pwrcntr.exe!0x401c40`: `dest=0x98, len=2, data={0x1e, arg}`.
Sites: `0x401c81, 0x41398c, 0x413a08, 0x413af1, 0x413ba0, 0x413cf3, 0x413e14,
0x41b626, 0x41cdc2, 0x47d56c` (the last is a `data={0x00}` poll).

---

## 3. Pentair IntelliFlo VS — `iFloNetIO(msg)` `[CONFIRMED]`

The master speaks the Pentair pump over a **separate primitive** `iFloNetIO`
(`NetIO.dll!0x3215`, the `_iFloVS` 0xA5 family) with a **different, smaller struct**
(built on the stack at `[ebp-0x14]`):

```
struct iFloMsg {
  +0 : u8 cmd       // 0x01 / 0x04 / 0x05 / 0x06 / 0x02
  +1 : u8 value     // index / argument
  +2 : u8 dest      // instance, or 0xFF broadcast
  ... msg.len = 3   // stored at [ebp-2]
}
```

10 `iFloNetIO` call sites; recovered command bytes (cited `Pwrcntr.exe`):

| cmd | builder | struct | meaning (matches project) |
|----:|---------|--------|---------------------------|
| **`0x04`** | `0x416592`→`0x4165cf` | `{0x04, 0x01, 0xFF}` len 3 (pump idx via `[idx+0x540c40]`) | **run/stop** |
| **`0x06`** | `0x4165f0`→`0x416624` | `{0x06, 0x01, 0x04}` len 3 | **set RPM** |
| **`0x01`** | `0x416734`→`0x41676e` | `{0x01, …}` | **set speed** |
| **`0x05`** | `0x4167a9`→`0x4167c2` | `{0x05, …}` | **mode** |
| **`0x06`** | `0x4167fd`→`0x416816` | `{0x06, …}` | set RPM (2nd site) |
| **`0x02`** | `0x4168e1` | `{0x02, …}` | run/program select |
| (more) | `0x4166f8, 0x416e60, 0x416eb5, 0x416f2a` | mix of `0x03/0x06` | status/req variants |

These **confirm the project's IntelliFlo command bytes**
`0x01 setspeed / 0x04 run-stop / 0x05 mode / 0x06 setRPM / 0x07 req-status`. The
`iFloNetIO` path wraps them in the Pentair `0xA5` framing inside `NetIO.dll`. A
separate RX buffer `iFloNetRxBuf` is imported for the pump replies. `[CONFIRMED]`

**Jandy ePump (`A`–`E` ASCII)** — the master uses the `ePumpAcNetIO`/`_ePumpAc`
family in `NetIO.dll` (`netSendMsg_ePumpAc 0x37b3`, `GetMasterMessage_ePumpAc
0x3a25`); the ASCII `'A'..'E'` (`0x41`–`0x45`) command bytes from the correlation
data are produced inside that NetIO family, not as literals in Pwrcntr.exe.
`[INFERRED — not pinned to a Pwrcntr literal]`

---

## 4. The probe / bus-enumeration mechanism `[CONFIRMED]`

There is **no separate "scan all addresses" routine**; enumeration is *implicit in
every command*:

* The master **broadcasts to `dest = 0xFF`** and retries across **instances 0..4**
  (`cmp 5`), breaking on the first reply (`netIO`→1). The responding device writes
  its **own address back** into `msg+0`, which the master then validates against the
  expected device class (`cmp eax, <class>`: heater `0x0d`, iAQ `0x1d`, …).
* So a device is "discovered" the first time it answers a command of its class; the
  per-slot stat counters in `NetIO` (`slot+0x85/+0x89/…`) track presence/health.
* The **GetId/login** command (`0x14`, `Pwrcntr.exe!0x401f9a`) is the explicit
  identity probe (loops 0xFF×5, copies the returned model string to `0x559340`).
* The **`0x00`** command (`0x421f73`) is the bare keep-alive poll.
* `dest = 0xFF` is the template's reserved/broadcast address (§3,
  `RemoteLogOn/LogOff` global flag) — used here as "poll this class".

Poll-cycle accounting: `IncPollCount`/`PollsDone`/`ClearPollCount`/`CountStartTm`
(NetIO) bracket each cycle so the master can report polls-per-second.

---

## 5. Config / options model `[CONFIRMED]`

The Start routine (`Pwrcntr.exe!0x4255f1`) reads an INI/registry **`Options`**
section before `netInit`:

| key | use | cite |
|-----|-----|------|
| **`CommPort`** | serial COM port number for the bus | `0x4256f8`/`0x425704` |
| **`ControllerType`** | controller model; default `0x7225` if `< 0xb54` | `0x42576b`/`0x4257d3` |
| **`NO_SLEEP_DELAYS`** | debug: skip inter-frame guard delays | `0x425663` |

`ControllerType` is decoded by `Pwrcntr.exe!0x41e5ba`: it sets the controller
capability globals — `byte[0x558ee3]=7` / `byte[0x558ed9]` (aux-relay count: 3/5/6/
10/14 via the jump table at `0x41e71a`, indexed `type-0x7226`) / `byte[0x558ed8]=1`
/ `byte[0x558ee2]` (a model flag) — i.e. **how many aux relays / which RS-x model**
(RS-4/6/8/…); `0x7234` is a special PDA/AquaPalm type. This is the controller's
equipment-capacity configuration that determines how many aux devices the master
polls. `[CONFIRMED]`

After config, `netInit` (`0x4256ec`) brings up `NetIO`, and the actual polling runs
on a worker thread (MFC `AfxBeginThread` wrapper around `CreateThread` at
`0x4ba64e`) that repeatedly invokes the §2 command builders.

---

## 6. Net for the project

* **Confirms from the master side**: the `0x00` poll, `0x14` GetId, heater `0x0C
  enable+setpoint` (responder class `0x0d`), the IntelliFlo `0x01/0x04/0x05/0x06`
  command bytes, the aux `0x08/0x09` relay sets, and the **entire iAQ page protocol**
  (`0x23` PageStart w/ page-type `0x2a`=EquipmentStatus, `0x24` PageButton, `0x26`
  PageMessage, `0x28` PageEnd, `0x31` ControlReady) — independently validating
  `iaq_protocol.md`.
* **New / useful**: the iAqualink2 WiFi-module application command name table
  (`ENQ/SSID/NETKEY/.../ONETOUCH_INFO/IPHONE_INFO`, §2d); the master's
  **broadcast-0xFF + instance-0..4 + break-on-reply** poll idiom; the class-6 (0x30-
  0x33) 450 ms timing; and the `ControllerType`→aux-relay-count config model.
* **Master ≠ slave struct**: master TX uses `{dest, u16 len, cmd, payload}` via
  `netIO`; the responder's address/length come back *in the same buffer*. Pentair
  uses the smaller `{cmd, val, dest}` via `iFloNetIO`.

### Confidence
CONFIRMED: every cmd byte / length / payload immediate cited to a Pwrcntr VA; the
`netIO` struct layout and TX/RX semantics; the 0xFF-broadcast probe idiom; the iAQ
command-name table; the Options/ControllerType config. INFERRED (flagged inline):
the exact device class behind a few responder-readback codes (`0x1d`, `0x98` for
iAQ), and that the ePump `A`–`E` bytes originate in `NetIO!_ePumpAc` rather than a
Pwrcntr literal.
