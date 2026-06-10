# Protocol intel — `Aquatemp.exe` (LX Heater) and `wtrMatic.exe` (JANDY CHEMLINK)

Static RE of the official Jandy *Alwin32* simulator suite, same method/citation style as
`docs/alwin32_simulator_protocol.md` (`binary!RVA`). Both binaries are thin UI shells over
`NetIO.dll` (`RemoteGetNextAddress`/`RemoteLogOn`/`GetMasterMessage`/`RemoteStatus`).
Cited RVAs use the loaded VA (ImageBase `0x400000`). **CONFIRMED** = read directly from the
disassembly; **INFERRED** = deduced from layout/labels but not 100% pinned.

> Headline: the install-doc table had the two **swapped/mislabelled**. `Aquatemp.exe`'s own
> resources say **"Aquatemp / LX Heater"** — it emulates the **gas-heater control board**
> (temperatures + switch inputs), *not* chemistry. The actual ChemLink (pH/ORP/salt/level acid
> feeder) is **`wtrMatic.exe`**, whose title bar literally reads **"JANDY CHEMLINK"**.

---

## 1. `Aquatemp.exe` — "Aquatemp / LX Heater" (gas-heater board)

### Identity (CONFIRMED)
* Resource strings: `"Aquatemp / LX Heater"`, `"Meas Temp:"`, `"Pool SP:"`, `"Spa SP:"`,
  `"Deg C"`, `"Heating"`, `"Off Line"`, `"Heat Cmd"`, `"Errors"`, and switch inputs
  `"Press Sw" / "Hi Lim Sw" / "Ext Sw 1" / "Air Flo Sw" / "Gas Sw" / "Ext Sw 2" / "Temp Sr"`
  (dialog `IDD=101`, `Aquatemp.exe` `.rsrc`). Window class `CAquatempView`, version "1.0".
* Imports `RemoteGetNextAddress, GetMasterMessage, RemoteStatus, RemoteLogOn, RemoteLogOff`.

### Address (CONFIRMED)
* `RemoteGetNextAddress(7)` — class **7**, `push 7` at `Aquatemp.exe!0x40137d`, call at
  `0x4013ee`. ⇒ `addr = (7<<3)|instance` = **`0x38`–`0x3B`**.
* The negotiated address is stored at object `+0xB8`; instance bits (`and al,7`) at `+0xB2`
  (`0x4013f3`/`0x401400`).
* **Cross-check:** matches the project's `DeviceClasses::LX_Heater = {0x38,0x39,0x3A,0x3B}`
  (`src/jandy/devices/jandy_device_types.h:54`). CONFIRMED identity = **LX Heater**.

### Log-on (CONFIRMED)
* `RemoteLogOn(handle=[+0x1C], addr=[+0xB8], initBuf=&[+0xC7])` at `0x4014b3`.
* Init buffer `[+0xC7..]` = **`{0x01, 0x00, 0x00}`** (`[+0xC7]=1`,`[+0xC8]=0`,`[+0xC9]=0`;
  `0x401487`/`0x401481`/`0x401491`). Same idle shape as the chlorinator.

### Inbound master→device commands (CONFIRMED)
Handler at `Aquatemp.exe!0x4014be`; pops the master message into `&[+0xB9]` via
`GetMasterMessage` (`0x4014e1`), then dispatches on `data[0]` = `[+0xB9]`:

| cmd  | meaning | payload (`data[1..]`) | citation |
|----:|---------|-----------------------|----------|
| `0x00` | poll / keep-alive | — (replies with 3-byte idle frame) | `0x4014f8`,`0x4015f9` |
| `0x0C` | **heater control** — master pushes setpoints + mode + (optional) measured temp | `[flags][PoolSP][SpaSP][MeasTemp?]` | `0x401502`,`0x40150a`+ |
| other | ignored (falls through, no reply) | | `0x401504 jne 0x401613` |

`0x0C` payload byte map (inbound buffer base `+0xB9` ⇒ `data[1]=+0xBA … data[4]=+0xBD`):

| byte | dst field | meaning | citation |
|-----:|-----------|---------|----------|
| `data[1]` (`+0xBA`) | `bl` (flags) | bit-packed control flags (below) | `0x401512` |
| `data[2]` (`+0xBB`) | `+0xAE` (**Pool SP**) | pool setpoint, sign-extended | `0x40150a→0x401518` |
| `data[3]` (`+0xBC`) | `+0xB0` (**Spa SP**) | spa setpoint, sign-extended | `0x40151f→0x401527` |
| `data[4]` (`+0xBD`) | `+0x98` (**Meas Temp**) | measured water temp, only when flags bit4 set; else `+0x98 = 0xFFFE` | `0x401580`/`0x401591` |

**`data[1]` flags byte** decode (`0x40152e`–`0x40157b`, CONFIRMED):

| bits | field (DDX control) | meaning |
|------|--------------------|---------|
| 0–1 (`bl&3`) | `+0x9E`/`+0xAA` (Pool/Spa Mode) | `1`→Pool Mode, `2`→Spa Mode |
| 2 (`0x04`) | `+0x80` (**Deg C**) | units: set = Celsius |
| 3 (`0x08`) | `+0xB4` (**Heat Cmd**) | heater demand on/off |
| 4 (`0x10`) | — | "Meas Temp present": if set, `data[4]` carries the measured temp |

### Outbound device→master responses (CONFIRMED)
Both sent via `RemoteStatus(addr, dataPtr, len, sendNow=1)` (thunk `0x4017ac` →
`NetIO.dll!RemoteStatus`):

| trigger | resp cmd (`data[0]`) | len | payload layout | citation |
|---------|:--------------------:|:---:|----------------|----------|
| poll `0x00` | `0x01` | 3 | `[0x01][0x00][0x00]` (idle echo of init buf `&[+0xC7]`) | `0x4015f9`–`0x40160b` |
| ctrl `0x0C` | **`0x0D`** | 4 | `[0x0D][status][MeasTemp][switchBits]` (`&[+0xC3]`) | `0x40159a`–`0x4015b7` |

`0x0D` payload bytes:

* `data[1]` = **status byte** `[+0xC4]`:
  * bit3 `0x08` = **Heating** (`+0x90`!=0, `0x40168c`)
  * bit4 `0x10` = **Off Line** (`+0x9A`!=0, `0x4016b1`)
  * low 3 bits = mode echo (cleared then set from inbound `bl&3`; `and [+0xC4],0xF8` `0x4015bc`)
* `data[2]` = **Measured Temp** `[+0xC5]` ← `[+0x98]` (sentinel `0xFFFE`/`0xFFFD` when no reading)
  (`0x40159a`/`0x4015a1`)
* `data[3]` = **switch/error bitfield** `[+0xC6]`, built at `0x401629`–`0x40166c`:

  | bit | mask | field (label) |
  |----:|-----:|---------------|
  | 0 | `0x01` | `+0xA2` **Press Sw** (pressure switch) |
  | 1 | `0x02` | `+0x94` **Hi Lim Sw** (high-limit) |
  | 2 | `0x04` | `+0x84` **Ext Sw 1** |
  | 3 | `0x08` | `+0x7C` **Air Flo Sw** (air-flow / fan) |
  | 4 | `0x10` | *(unused here)* |
  | 5 | `0x20` | `+0x8C` **Gas Sw** |
  | 6 | `0x40` | `+0x88` **Ext Sw 2** |
  | 7 | `0x80` | `+0xA6` **Temp Sr** (temp-sensor fault) |

### Field ↔ UI control map (CONFIRMED via DDX `0x401235`–`0x401360` + dialog 101)
`DDX_Check`=`0x417e0c`, `DDX_Text`=`0x417dd1`.

| field | ctrl id | label | | field | ctrl id | label |
|-------|--------:|-------|-|-------|--------:|-------|
| `+0xAE` | 1000 | Pool SP | | `+0xA2` | 1014 | Press Sw |
| `+0xB0` | 1001 | Spa SP | | `+0x94` | 1015 | Hi Lim Sw |
| `+0x98` | 1002 | Meas Temp | | `+0x84` | 1016 | Ext Sw 1 |
| `+0x9E` | 1009 | Pool Mode | | `+0x7C` | 1017 | Air Flo Sw |
| `+0xAA` | 1010 | Spa Mode | | `+0x8C` | 1018 | Gas Sw |
| `+0x80` | 1011 | Deg C | | `+0x88` | 1019 | Ext Sw 2 |
| `+0x9A` | 1012 | Off Line | | `+0xA6` | 1020 | Temp Sr |
| `+0x90` | 1013 | Heating | | `+0xB2` | 1021 | Unit # |
| `+0xB4` | 1022 | Heat Cmd | | | | |

### Net for the project
* Confirms LX-Heater address `0x38`–`0x3B`.
* **Master→heater is `0x0C`** carrying `[flags][PoolSP][SpaSP][MeasTemp?]`; **heater→master is
  `0x0D`** = `[status][MeasTemp][switchBits]`. Idle/poll reply is cmd `0x01`, len 3.
* `Heating = 0x08` in the status byte **matches** the project's
  `HeaterStates::Heating = 0x08` (`heater_message_status.h:18`). Note the project's
  `HeaterMessage_Status` reads state at index 4 / error at index 6 of a *different* heater
  frame — the LX/Aquatemp sim's status byte is `data[1]` of a 4-byte `0x0D`. Worth reconciling
  the heater frame variants against a capture; the **switch bitfield** (Press/HiLim/Ext1/
  AirFlo/Gas/Ext2/TempSr at bits 0,1,2,3,5,6,7) is new detail vs the project's
  `HeaterErrors` enum (`SensorFault 0x02 / AuxMonitor 0x08 / HighLimit 0x10`).

---

## 2. `wtrMatic.exe` — "JANDY CHEMLINK" (pH/ORP/salt acid-feed controller)

### Identity (CONFIRMED)
* Resources: title **`"JANDY CHEMLINK"`** (dialog `IDD=102`), `"CHEMLINK ADDR"`,
  `"ORP SETUP"`, `"PH SETUP"`, `"CHEMLINK OFF"/"CHEMLINK 1"/"CHEMLINK 2"`. PDB path
  `c:\Projects\RevQ\Alwin32\wtrMatic\Debug\wtrMatic.pdb`.
* LCD/sprintf field set (chemistry feeder): `"SALT"`, `"NEXT CLEAN %s %d"`, `"STOP PH %s"`,
  `"WAIT PH %s"`, `"LOW ALERT %d"`, `"HI ALERT %d"`, `"SET POINT %d"`, `"DELAY OT %d MIN"`,
  `"OVERFEED %d MIN"`, `"FEED TM %s"`, `"FEEDER %s"`, `"CAL AT PH %d"`, `"ACID/BASE %s"`,
  `"LEVEL SWITCH: %s"`, `"CONT"`, `"NONE"` (`wtrMatic.exe!0x415764`–`0x415a74`). Internal
  buffer symbols `phBuf`, `orpBuf`, `rxBuf` survive in the debug build.
* Imports only `RemoteStatus, GetMasterMessage, RemoteLogOn, RemoteLogOff` — **no**
  `RemoteGetNextAddress` ⇒ address is **fixed/configured**, not bus-negotiated.

### Address (CONFIRMED)
* Address byte stored in the slot at object `+0x74`; selected from a 3-entry table indexed by
  `[+0xC8]` (the OFF/1/2 selector): `al = byte[index + 0x42A8EF]` (`0x415E0F`), then
  `[+0x74] = al` (`0x415E15`). When index `0` ⇒ logs off (`0x415DD7`).
* Table bytes at `wtrMatic.exe!0x42A8EF` = **`00 80 81`** ⇒ **CHEMLINK 1 = `0x80`,
  CHEMLINK 2 = `0x81`** (index 0 = OFF).
* `RemoteLogOn(handle=[+0x20], addr=[+0x74], initBuf=&[+0xBD])` at `0x415E2F` (init buf = the
  3-byte ACK buffer).
* **Cross-check:** matches the project's `DeviceClasses::Chemlink = {0x80,0x81,0x82,0x83}`
  (`jandy_device_types.h:65`). CONFIRMED identity = **ChemLink**, and the sim only exposes the
  first two instances `0x80`/`0x81`.

### Inbound master→device commands (CONFIRMED — jump-table dispatch)
Handler at `wtrMatic.exe!0x4153C0`; `GetMasterMessage` into `&[ebp-0x30]` (`0x41540D`), command
byte `data[0]=[ebp-0x30]`, range-checked `<= 0x20` then a **256-way switch** via byte-map
`@0x415940` → jump table `@0x415910` (`0x415454`/`0x41545B`). Decoded table:

| cmd | handler | action | resp cmd | resp len | citation |
|----:|---------|--------|:--------:|:--------:|----------|
| `0x00` | `0x415653` | poll / keep-alive | `0x01` | 3 | `0x415653` |
| `0x01` | `0x415706` | **set/get block @ struct+0x86** (write payload, read back 11B) | `0x22` | 14 | `0x415706` |
| `0x02` | `0x4157DF` | set/get @ struct+0x79 (returns 2 packed bytes) | `0x21` | 7 | `0x4157DF` |
| `0x03` | `0x415462` | set field @ struct+0x7A | `0x01` | 3 | `0x415462` |
| `0x04` | `0x4154C7` | set field @ struct+0x85 | `0x01` | 3 | `0x4154C7` |
| `0x05` | `0x41552E` | set field @ struct+0x92 | `0x01` | 3 | `0x41552E` |
| `0x06` | `0x41558D` | set field @ struct+0x93 | `0x01` | 3 | `0x41558D` |
| `0x09` | `0x41577A` | set @ struct+0x77, return 1 byte (`[+0x95]`) | `0x21` | 3 | `0x41577A` |
| `0x0A` | `0x4155EC` | set field @ struct+0x94 | `0x01` | 3 | `0x4155EC` |
| `0x18` | `0x41586E` | set field @ struct+0x97 | `0x01` | 3 | `0x41586E` |
| `0x20` | `0x415692` | **read main status block** (write @+0x75, read back 10B) | `0x22` | 12 | `0x415692` |
| all others (`0x07,0x08,0x0B–0x17,0x19–0x1F`) | `0x4158C8` | ignored (no reply) | — | — | byte-map `idx 11` |

Each handler is the same shape: `memcpy(&struct[off], &inbound[0], rxLen=[+0xC0])`
(`memcpy` = `0x411758`), then build a reply at a per-command buffer and
`RemoteStatus(addr=[+0x74], replyPtr, len, 1)` (`0x411636`). The device is a **register-file**:
each command writes the inbound payload to a fixed offset of one contiguous shadow struct
(`~+0x75`), and the read commands return a window of it.

### Outbound device→master responses (CONFIRMED)
Three response command bytes (= `data[0]`):

* **`0x01` — generic ACK** (len 3): `[0x01][echoed_cmd][0x00]`. Used by `0x00,0x03,0x04,0x05,
  0x06,0x0A,0x18`. `data[1]` echoes the master command number (e.g. `[+0xBE]=3` for cmd `0x03`,
  `0x415497`).
* **`0x21` — short data reply**:
  * cmd `0x09` (len 3): `[0x21][0x09][value=+0x95]` (`0x41579B`–`0x4157B8`).
  * cmd `0x02` (len 7): `[0x21][0x02][0x00][+0x7E][0x00][+0x89][0x00]` — two readings
    interleaved with zero pad bytes (`0x415800`–`0x415846`). INFERRED: the two payload bytes
    are a pH and an ORP/cal reading (the `0x00` pads look like high bytes of 16-bit fields).
* **`0x22` — full status block**:
  * cmd `0x20` (len 12): `[0x22][0x20][10 bytes copied from struct+0x7B]`
    (`0x4156B3`–`0x4156E1`). The `0x20` is a fixed sub-type byte at `[+0x9A]`.
  * cmd `0x01` (len 14): `[0x22][0x01][11 bytes copied from struct+0x86]`
    (`0x415727`–`0x415756`). Sub-type byte `[+0xA6]=0x01`.

  ⇒ the **main ChemLink status payload command is `0x22`**, carrying a sub-type selector
  (`0x20` or `0x01`) followed by a block of chemistry readings/config (pH, ORP, salt, level,
  setpoints, feed timers — the LCD field set). Exact byte→field offsets within the 10/11-byte
  block are **INFERRED** (the shadow-struct offsets `+0x7B`/`+0x86` correspond to the same
  regions written by the set-commands above) and would need a live capture to pin to specific
  pH/ORP/salt values.

### Notable strings (field semantics, CONFIRMED present; offsets INFERRED)
`SALT`, `NEXT CLEAN`, `STOP PH`, `WAIT PH`, `LOW ALERT`, `HI ALERT`, `SET POINT`, `DELAY OT`,
`OVERFEED`, `FEED TM`, `FEEDER`, `CAL AT PH`, `ACID/BASE`, `LEVEL SWITCH`, `CONT`, `NONE`,
`ORP SETUP`, `PH SETUP`. These name the readable fields the `0x22`/`0x21` blocks transport:
pH/ORP set-points and alerts, acid/base feeder mode + feed/overfeed/delay timers, salt level,
a level (water/tank) switch state, and a "next clean"/calibration schedule.

### Net for the project
* Confirms ChemLink address `0x80`/`0x81` (sim only emulates the two-instance pair).
* The project's `chemlink_message_response.h` / `chemlink_device.h` should expect a response
  **command `0x22`** with a **sub-type byte** (`0x20` = main status, `0x01` = secondary block),
  plus a short **`0x21`** reply and a generic **`0x01` ACK**. Master polls with `0x00`; the rich
  pull is master cmd `0x20` (→ `0x22/0x20`, 12B) and `0x01` (→ `0x22/0x01`, 14B). Config writes
  are masters `0x02–0x18` (acid/base, setpoints, timers, level), each ACKed with `0x01`.
* **CAVEAT (INFERRED):** the per-byte layout of the `0x22` 10/11-byte payloads (which byte is pH
  vs ORP vs salt vs level) is not pinned from statics — the sim just `memcpy`s a struct window.
  Capture-validate against a real ChemLink.

---

## Method / reproduction
* `alw.py info|strings|disasm|callsites` against `C:\Program Files (x86)\Alwin32\*.exe`.
* Dialog/control map: `findings/dlgdump2.py` (classic `DLGTEMPLATE`, used for Aquatemp;
  wtrMatic uses `DLGTEMPLATEEX` + custom-drawn LCD so its labels come from the sprintf
  strings, not DDX).
* Jump table decode: `findings/jtab.py <exe> <jumptableVA> <bytemapVA> <maxcmd>`.
* Raw data bytes: `findings/rdbytes.py <exe> <VA> <len>`.
* Thunk chains resolved: Aquatemp uses direct `FF25` IAT thunks (`0x4017A6`=LogOn,
  `0x4017AC`=RemoteStatus, `0x4017B2`=GetMasterMessage). wtrMatic uses MSVC ILT `E9` →
  `FF25` IAT double-thunks (`RemoteStatus` IAT `0x42BDD8`, `RemoteLogOn` IAT `0x42BDE0`,
  `GetMasterMessage` IAT `0x42BDDC`, `memcpy` IAT `0x42BD08`).
