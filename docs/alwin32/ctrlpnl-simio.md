# `Ctrlpnl.exe` and `Simio.exe` — iAqualink wired Control Panel & I/O Simulator

Reverse-engineered from the official Jandy *Alwin32* simulator suite
(`C:\Program Files (x86)\Alwin32`, debug MFC builds, VS2003 / `mfc71d.dll`).
Method: static x86 disassembly + PE/resource parsing (pefile + capstone, via `alw.py`
and ad-hoc scripts). Every claim is cited as `<exe>!0xRVA` (image base `0x400000`, so
VA `0x40xxxx` == RVA `0xxxxx`) or `iodll.dll!0xRVA` (image base `0x10000000`).

Cross-checked against `docs/alwin32_simulator_protocol.md` (§2 frame, §3 addressing, §4
network-table model) and `docs/alwin32/panels.md` (the keypad button/LED protocol).

**Legend:** `[CONFIRMED]` = directly observed in code/data. `[INFERRED]` = deduced from
data flow / not pinned to wire bytes.

---

## Part 1 — `Ctrlpnl.exe` : the iAqualink wired Control Panel

### Identity `[CONFIRMED]`
* FileDescription **"CTRLPNL MFC Application"**; document type string **"IAqualink Control
  Panel"** (`Ctrlpnl.exe!0x31d98` W); window/app strings "Ctrlpnl Windows Application";
  **Application Version 5.0** (`0x2e54a` W).
* Imports from `NetIO.dll`: `GetMasterMessage, RemoteStatus, CommInUse, RemoteLogOn,
  RemoteLogOff, RemoteGetNextAddress` — i.e. a **bus panel** in the same family as
  `Smallcp.exe` (the "Small Control Panel" / OneTouch), but the **full all-button iAqualink
  keypad** found at the equipment pad. Uses VBX button controls (`BTNVBX10.VBX`,
  `THREED.VBX`) — `0x2f2c2`/`0x2f37a` W.

This is the richer cousin the prompt expected: it has a **16-char alphanumeric display**
(7-segment decode, like Smallcp), **~24 LED indicators**, and **27 buttons** (vs 8/9 on the
all-button keypads).

### Address — class `0x01`, base `0x08` `[CONFIRMED]` — NEW
`Ctrlpnl.exe!0x401297` `push 1` → `0x4012ff` `call RemoteGetNextAddress` (thunk `0x402506`
→ IAT `0x430874`). Result stored at `[esi+0xef]` (`0x401305 mov [esi+0xef], al`).

| field | value |
|------|------|
| class arg to `RemoteGetNextAddress` | **`0x01`** |
| address range | **`0x08`–`0x0F`** (`(0x01<<3)|instance`, instances 0–3) |

This is a **new device class not previously in the project map** (panels.md has keypad
classes `0x02`/`0x04` and OneTouch `0x08`; the equipment-pad iAqualink Control Panel is
class **`0x01`**, base **`0x08`**). Stored RS-485 address byte lives at object `+0xef`.

### Inbound (master → panel) — `GetMasterMessage(addr=[+0xef], inBuf=[+0xc4])` `[CONFIRMED]`
Handler `Ctrlpnl.exe!0x4018d6`. Calls `GetMasterMessage` at `0x4018f8` (thunk `0x402524`),
length stored to `+0xd7`, `inBuf` at `+0xc4`. `inBuf[0]` = command (`movzx eax,[ebp]`,
ebp=`+0xc4`); `inBuf[1]` saved to `bl`. Dispatch chain at `0x40191f`–`0x401951`
(`sub eax,0 / dec;dec / dec / dec / dec / sub eax,3 / dec`):

| cmd | handler | meaning | cite |
|----:|:-------:|---------|------|
| `0x00` | `0x401b52` | **poll** — send 3-byte status (returns pending button), clear button | `0x401b52`–`0x401b9d` |
| `0x02` | `0x401b0d` | **LED state** — ACK; `memcmp(inBuf@+0xc4, cache@+0xe9, 6)`; on change `memcpy` 6 bytes & redraw via LED applier `0x401e01` | `0x401b0d`–`0x401b50` |
| `0x03` | `0x401a8e` | **set display value / clear-bit** — ACK; `and [+0xda],0xfe`; 7-seg decode `inBuf` and store to display field `+0xf2` | `0x401a8e`–… ; seg decode `0x4018ae` |
| `0x04` | `0x401a0c` | **set display item / set-bit** — `or [+0xda],1`; ACK; update display index from `inBuf[1]` (`+0xf2`), arm a 2000 ms (`0x7d0`) timer `0x65` | `0x401a0c`–`0x401a89` |
| `0x05` | `0x401b52` | **toggle/poll** (shares poll path) | `0x401945` |
| `0x08` | `0x4019ac` | **cursor / selected-item** — ACK; reconcile display cursor word `+0xf6` from `inBuf[1..2]`, redraw `0x40179b` | `0x4019ac`–`0x401a07` |
| `0x09` | `0x401957` | **status request / rich reply** — if `inBuf[1]!=0` run `0x4021ec`/`0x402222`; then build a richer response: `[+0xdc]=0x0e`, `[+0xdd]=[+0xf8]`, `[+0xde]=[+0xf9]`, then send 3-byte status | `0x401957`–`0x4019a7` |
| else | `0x401b92` | default (clear button, no-op) | |

This is the **same OneTouch-class vocabulary as `Smallcp.exe`** (poll/LED/display/
cursor), confirming Ctrlpnl is a OneTouch-protocol panel — just at class `0x01`/base `0x08`
with a larger button/LED set. The display uses the identical 7-segment remap
(`Ctrlpnl.exe!0x4018ae`: `0x7e→0x3e`, `0x7f→0x3c`, `+0x60 entries →0xb0`, …) seen in
`Smallcp.exe!0x401e64`.

**LED byte layout (cmd `0x02`) `[CONFIRMED]` — 2 bits per LED.** Applier
`Ctrlpnl.exe!0x401e01` decodes the 6-byte cache (`inBuf[1..6]` at `+0xe9`) as **4 LEDs ×
2 bits per byte** (`and al,3` / `shr al,2` / `shr al,4` / `shr al,6`), each routed to a
dialog LED control via `0x401d64`:

| cache byte | LED ctrl IDs (4 per byte) |
|:----------:|---------------------------|
| `+0xea` (`inBuf[1]`) | `0x3f9, 0x40b, 0x3f1, 0x3ef` |
| `+0xeb` (`inBuf[2]`) | `0x3ed, 0x3eb, 0x3e9, 0x3f5` |
| `+0xec` (`inBuf[3]`) | `0x3f3, …` (continues) |
| `+0xed…+0xef` | further LEDs (up to ~24 total) |

Value `0/1/2-3` = off / on / blink (same encoding as all panels — `docs/alwin32/panels.md`
§0). The 6 LED bytes carry ~24 indicators, matching the Pool/Spa/Solar/Heaters/Aux1–15
front-panel LEDs. `[INFERRED]` exact LED-ID → physical-label map beyond what the dialog
control IDs imply (VBX captions are not in the dialog template).

### Outbound (panel → master) — `RemoteStatus(addr=[+0xef], out=[+0xd9], len=3, now=1)` `[CONFIRMED]`
Sent from every command handler (8 `RemoteStatus` call sites, all `push 3`).
Fixed **3-byte** frame `out = [+0xd9][+0xda][+0xdb]`:

| byte | offset | value | cite |
|:----:|:------:|-------|------|
| `out[0]` | `+0xd9` | **`0x01`** (response cmd; set once at init) | `0x40145c` `mov byte [esi+0xd9],1` |
| `out[1]` | `+0xda` | **status bits** — bit0 set by cmd `0x04`, cleared by cmd `0x03` | `0x401a0c` / `0x401a92` |
| `out[2]` | `+0xdb` | **button code (0x01–0x1e), 0 = none** — cleared after each send | setter `0x401fd7`; clear `0x401b92` (`and [esi+0xdb],0`) |

A Ctrlpnl button press therefore goes on the wire as `[0x01 <status> <btnCode>]` from
address `0x08`. This is the **same button-press encoding as the simpler panels**
(`[0x01][b1][btn]`, small-int button index) — just a much larger code space.

> Note: cmd `0x09` additionally pre-loads a 3-byte block at `+0xdc` (`0x0e`, `[+0xf8]`,
> `[+0xde]=[+0xf9]`) — a **secondary `0x0e` response payload** distinct from the +0xd9
> status frame. `[INFERRED]` this is a second-status / extended reply; not pinned to a
> captured wire frame.

### Button table — codes `[CONFIRMED]`, labels `[CONFIRMED set]` / `[INFERRED per-code]`
`SetButton(code)` = `Ctrlpnl.exe!0x401fd7` (`al=arg; [ecx+0xdb]=al`). 27 constant-push
thunks (`push <code>; call 0x401fd7; ret`) at `0x4020bb`–`0x40218f` feed it; bound to dialog
controls through the MFC message map. Recovered **button codes**:

```
0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f 0x10
0x12 0x13 0x14 0x15 0x17 0x18 0x19 0x1a 0x1c 0x1d 0x1e
```

(27 codes; gaps at `0x11`, `0x16`, `0x1b`.) **Confirmed code↔label pairs** (from dialog
captions that survived as text — the few non-VBX controls):

| code | label | ctrl ID |
|----:|-------|--------|
| `0x10` | `<-` (left/prev) | `0x402` |
| `0x15` | `->` (right/next) | `0x405` |
| `0x1a` | `*` | `0x406` |
| `0x1e` | **Simulate** | `0x42a` |

**Button label SET `[CONFIRMED]`** (resource strings `0x2e86a`–`0x2eebe` W): `Pool`,
`Aux1`–`Aux15` (the full 15 aux), `Solar`, `Heaters`, `Menu`, `Cancel`, `Back`, `Enter`,
`Delay Override`, `Simulate`, plus nav `<-`/`->`/`*`. The 27 codes correspond to these.
The exact aux-number → code mapping is **`[INFERRED]`** (the aux buttons are VBX controls
whose captions are not in the dialog template, so the code↔aux# binding could not be pinned
without the VBX property data) — do not hard-code an aux→code table from this binary alone;
confirm against a live capture.

### Ctrlpnl vs the simpler panels (summary)
| panel | class | base | display | LEDs | buttons | outbound |
|-------|:-----:|:----:|---------|:----:|:-------:|----------|
| 2x4rem | `0x02` | `0x10` | none | 2 | 1–8 | `[0x01 0x00 btn]` |
| 8button | `0x04` | `0x20` | text (cmd 0x03) | ≥4 | 1–9 | `[0x01 0x00 btn]` |
| Smallcp (OneTouch) | `0x08` | `0x40` | 16-char + cursor | matrix | 1–7 | `[0x01 b1 btn]` |
| **Ctrlpnl (iAqualink CP)** | **`0x01`** | **`0x08`** | **16-char 7-seg + cursor** | **~24 (6 LED bytes)** | **27 (0x01–0x1e)** | **`[0x01 status btn]`** |

So **yes** — Ctrlpnl has both more buttons (27 incl. 15 aux + nav + Menu/Back/Enter) and a
display, and it speaks the OneTouch command set (`0x00/0x02/0x03/0x04/0x05/0x08/0x09`) at a
new address class `0x01` / base `0x08`.

### Provenance / re-derivation (Ctrlpnl)
RGNA class arg `0x401297` (`push 1`); logon `0x40171c` (`RemoteLogOn`); GetMasterMessage
`0x4018f8` (`inBuf=+0xc4`); handler `0x4018d6`; dispatch chain `0x40191f`–`0x401951`;
RemoteStatus `0x40199f`/`0x4019be`/… (`out=+0xd9, len=3`); out[0] init `0x40145c`; button
clear `0x401b92`; LED applier `0x401e01` (2-bit/LED, ctrl `0x401d64`); 7-seg decode
`0x4018ae`; SetButton `0x401fd7`, thunks `0x4020bb`+.

---

## Part 2 — `Simio.exe` : the PowerCenter I/O Simulator

### Role `[CONFIRMED]`
* FileDescription **"SIMIO MFC Application"**, doc type **"Power Center I/O"**
  (`Simio.exe!0x3041c` W), **Version 1.0** (`0x2d1fe` W).
* Imports **only `iodll.dll`** (no NetIO): `SetKeyPressed, SetOptions, SetOptions_S2,
  SetWaterDegF, SetSolarDegF, SetFreezeDegF, SetA2DCh, SetBatteryVolts, GetRelay, GetLED,
  voltsBattery, readDegF, adc_10bit, GetOptions, GetOptions_S2, RelaysChanged, LEDsChanged`.

Simio is **NOT a bus device.** It is the UI front-end for the **simulated PowerCenter
on-board hardware** in `iodll.dll`. The critical architectural fact:

> **Only `Pwrcntr.exe` imports BOTH `iodll.dll` and `NetIO.dll`** (every other sim imports
> only `NetIO.dll`; Simio imports only `iodll.dll`). `iodll.dll` keeps its hardware state in
> a PE **`Shared` section** (`iodll.dll` section "Shared" VA `0xf000`). So **Simio writes the
> simulated relays / LEDs / sensors / option DIP switches into shared memory, and the
> PowerCenter controller `Pwrcntr.exe` reads them** to decide its behavior and drive the
> RS-485 bus. Simio is the "set the hardware inputs / watch the outputs" panel.

### `iodll.dll` shared-memory hardware map `[CONFIRMED]`
All hardware state lives at fixed addresses in the `Shared` section (`0x1000f000+`):

| iodll export (RVA) | shared addr | meaning |
|--------------------|-------------|---------|
| `GetOptions 0x10e0` / `SetOptions 0x11e0` | `[0x1000f004]` (u8) | **Option DIP bank "S1"** |
| `GetOptions_S2 0x10f0` / `SetOptions_S2 0x11f0` | `[0x1000f008]` (u8) | **Option DIP bank "S2"** |
| `SetRelay 0x11a0` / `GetRelay 0x11c0` | `[0x1000f050 + idx]` (u8[]) | **relay states** (+ dirty flag `[0x1000f018]`) |
| `SetLED 0x1170` / `GetLED 0x1190` | `[0x1000f060 + idx]` (u8[]) | **front-panel LED states** (+ dirty `[0x1000f014]`) |
| `SetSolarDegF 0x1210` | `[0x1000f020]` (u16) | solar temp |
| `SetWaterDegF 0x1200` | `[0x1000f022]` (u16) | water temp |
| `SetFreezeDegF 0x1220` | `[0x1000f024]` (u16) | freeze/air temp |
| `SetA2DCh 0x1230` | `[0x1000f038 + ch*2]` (u16[]) | raw A/D channel values |
| `SetKeyPressed 0x11d0`, `SetBatteryVolts 0x1250`, `SetPowerFail 0x1260` | (various `0x1000f0xx`) | keypad ADC, battery, power-fail |

`Get*`/`Set*` are trivial load/store accessors (`mov al,[addr]` / `mov [addr],al`), e.g.
`GetOptions` = `iodll.dll!0x10e0` `mov al,[0x1000f004]; ret`; `SetOptions` = `0x11e0`
`mov al,[esp+4]; mov [0x1000f004],al; ret`. `GetOptions_S2`/`SetOptions_S2` =
`0x10f0`/`0x11f0` on `[0x1000f008]`.

### Option DIP-switch bit map `[CONFIRMED labels for S1]`
Simio's main dialog (resource **DIALOG 101 "Power Center I/O"**) has labelled groups
**"S1 Switches"**, **"S2 Switches"**, **"Spa-Side Ctrls"**, **"Relays"**, **"Temperatures"**,
**"Heat Pump"** (`Simio.exe!0x2dc0a`/`0x2dd82`/`0x2dbd2`/`0x2dc3a`/`0x2db9e` W). Each S1/S2
checkbox is a `WM_COMMAND/BN_CLICKED` handler that **XOR-toggles one bit** of the option
byte: `GetOptions → xor mask → SetOptions` (`Simio.exe!0x401bad` for S1, `0x401bd3` for S2;
each preceded by a `push <mask>` thunk). The bit `mask` and the checkbox **caption** are
joined via the MFC message map (entries at `0x4214f4`+, layout `{pfn, 0x111, code, nID,
nLastID, nSig}`) and the dialog template captions.

**S1 Switches — `GetOptions`/`SetOptions` byte `[0x1000f004]`** `[CONFIRMED]`
(all 8 bits, label = the captioned checkbox in the "S1 Switches" group box, x≈164):

| bit | mask | S1 option | ctrl ID | thunk |
|:---:|:----:|-----------|:-------:|:-----:|
| 0 | `0x01` | **No Cool** | `0x3f1` | `0x401c08` |
| 1 | `0x02` | **Calibrate** | `0x3f2` | `0x401bf8` |
| 2 | `0x04` | **CL JVA Asn** (chlorinator JVA assign) | `0x3f3` | `0x401c23` |
| 3 | `0x08` | **Heat Delay** | `0x3f4` | `0x401c10` |
| 4 | `0x10` | **Cleaner** | `0x3ee` | `0x401c00` |
| 5 | `0x20` | **Heat Pump** | `0x411` | `0x401c33` |
| 6 | `0x40` | (A/D-select radio "2", id `0x40a`) | `0x40a` | `0x401c2b` |
| 7 | `0x80` | **Spillover** | `0x3f0` | `0x401c18` |

> Notes / `[INFERRED]` caveats on S1:
> * The "S1 Switches" group box visually also shows **2 Speed** (id `0x3ef`) and **Share Htr**
>   (id `0x40d`), but those two controls are **not** wired to a plain XOR-toggle in the message
>   map (Share Htr `0x40d` → special handler `0x401cc4`; 2 Speed `0x3ef` has no toggle entry),
>   so they are not pinned to an S1 bit here.
> * Bit `0x20` is wired to the **"Heat Pump"** checkbox (id `0x411`, which sits in the S2 box
>   visually) and bit `0x40` to an A/D-select radio (id `0x40a`) — i.e. the simulator's UI
>   layout does not perfectly mirror the physical S1 switch order. Treat the **mask↔function**
>   pairs as authoritative (they are the literal bit the controller reads); the physical
>   switch-position numbering is `[INFERRED]`.

**S2 Switches — `GetOptions_S2`/`SetOptions_S2` byte `[0x1000f008]`** `[CONFIRMED masks]`
(5 of 8 bits wired to checkboxes; only one carries a surviving caption):

| bit | mask | S2 option | ctrl ID | thunk |
|:---:|:----:|-----------|:-------:|:-----:|
| 0 | `0x01` | (S2 sw 1) | `0x412` | `0x401c3b` |
| 1 | `0x02` | (S2 sw 2) | `0x413` | `0x401c43` |
| 2 | `0x04` | (S2 sw 3) | `0x414` | `0x401c4b` |
| 3 | `0x08` | (S2 sw 4) | `0x415` | `0x401c53` |
| 7 | `0x80` | (S2 sw 8) | `0x418/0x416/0x417` | `0x401c5b` |

The S2 checkboxes carry **no captions in the dialog template** (the labels are drawn as
separate static text not bound to the controls), so the S2 bit *meanings* are `[INFERRED]`.
The **menu/label string pool** that names the S2-class options is
`Share Htr` (`0x2da9e` W), `LV Heat` (`0x2daea` W), `Solar boost` (`0x2db2e` W),
`Heat Pump` (`0x2dc8e` W) — these are the candidate S2 option labels, but the exact
label→bit binding for S2 could **not** be confirmed from this binary (capture-gated).

### Other Simio simulation surfaces `[CONFIRMED]`
* **Relays** (group "Relays", `Simio.exe!0x2dc3a` W): the dialog shows relay checkboxes
  labelled (static text) `Pump`, `Elect Heat`, `Solar`, `Cleaner`, `Intake`, `Return`,
  `Aux 1`–`Aux 7` (`0x2d562`–`0x2d7ba` W). Each maps to a `SetRelay(idx,val)` →
  `iodll.dll [0x1000f050+idx]`. Simio reads them back with `GetRelay`; `RelaysChanged`
  (`iodll.dll!0x1150`) is the change flag.
* **Temperatures** (group, `0x2db9e` W): edit fields for `Water (or Pool)`, `Solar`,
  `Air`, with `(or Spa)`/`(or Solar)` annotations → `SetWaterDegF`/`SetSolarDegF`/
  `SetFreezeDegF`. A "Calibrate"/`(0-100%)` field and a 4-way **A/D channel selector**
  (radio buttons "1"/"2"/"3"/"4", ids `0x407`–`0x40a`) call `SetA2DCh` (thunks
  `0x401c66`/`0x401c6f`/`0x401c78`/`0x401c81` push channels `0x0d`–`0x10`).
* **Battery / Volts** (`0x2d96e`/`0x2d996` W) → `SetBatteryVolts` / `voltsBattery`.
* **Spa-Side Ctrls** (`0x2dbd2` W) — spa-side remote inputs.
* Aux relay labels go up to **Aux 7** here (the I/O board's direct relays); the higher
  aux numbers on `Ctrlpnl` are expansion/relay-board aux, not direct PowerCenter relays.

### Provenance / re-derivation (Simio + iodll)
iodll accessors: `GetOptions 0x10e0`→`[0x1000f004]`, `SetOptions 0x11e0`,
`GetOptions_S2 0x10f0`→`[0x1000f008]`, `SetOptions_S2 0x11f0`, `SetRelay 0x11a0`/`GetRelay
0x11c0`→`[0x1000f050+i]`, `SetLED 0x1170`/`GetLED 0x1190`→`[0x1000f060+i]`,
`SetWaterDegF 0x1200`→`f022`, `SetSolarDegF 0x1210`→`f020`, `SetFreezeDegF 0x1220`→`f024`,
`SetA2DCh 0x1230`→`[0x1000f038+ch*2]`. Simio S1 toggle `Simio.exe!0x401bad` (calls
`GetOptions 0x401d60`, `SetOptions 0x401da2`); S2 toggle `0x401bd3` (`GetOptions_S2 0x401d5a`,
`SetOptions_S2 0x401da8`); bit thunks `0x401bf8`–`0x401c65`; message map `0x4214f4`+; dialog
101 captions from `.rsrc`. iodll/NetIO importer survey across all 23 exes confirms
**Pwrcntr.exe = the sole iodll∧NetIO bridge**.

---

## Cross-validation vs the project / takeaways

* **NEW address class for `jandy_device_types.h`:** class **`0x01`**, base **`0x08`** =
  **iAqualink wired Control Panel** (`Ctrlpnl`). OneTouch command set, 27 buttons, ~24 LEDs,
  16-char display. Outbound frame `[0x01][status][buttonCode]`, button codes `0x01–0x1e`.
* **Confirms** the uniform panel button/LED protocol from `panels.md`: inbound LED cmd
  `0x02` = 2 bits/indicator; outbound = fixed 3-byte `[0x01][b1][btn]` with a small-int button
  index. Ctrlpnl extends it with the full OneTouch display vocabulary at a new class.
* **Option DIP model (Simio ↔ Pwrcntr via iodll shared memory):** two 8-bit banks **S1**
  (`GetOptions`) and **S2** (`GetOptions_S2`). S1 bits **confirmed**: `0x01`=No Cool,
  `0x02`=Calibrate, `0x04`=CL JVA Asn, `0x08`=Heat Delay, `0x10`=Cleaner, `0x20`=Heat Pump,
  `0x80`=Spillover (bit `0x40` = sim A/D-select artifact). S2 bit *labels* are capture-gated
  (`[INFERRED]` candidates: Share Htr / LV Heat / Solar boost / Heat Pump). These options are
  **controller-internal config** read by the PowerCenter firmware (`Pwrcntr.exe`) — they do
  **not** appear directly on the RS-485 wire, so they are background config context rather than
  a new message to decode.
