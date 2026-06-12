# Heater protocol intel — `HeatPump.exe` & `LXI Heater.exe` (Alwin32 simulator suite)

Static RE of the two Jandy heater sims in `C:\Program Files (x86)\Alwin32`, using the
shared NetIO device model documented in `docs/alwin32_simulator_protocol.md`. Both are MFC
debug builds; every claim is cited `binary!RVA` (RVA = file VA, ImageBase `0x400000`).
**CONFIRMED** = read directly from disassembly/resources. **INFERRED** = reasoned, flagged.

> **Headline:** both heaters are built from the *same Aquatemp codebase* (resource class
> `CAquatempDoc`/`CAquatempView`, title "AQUATEMP"). They differ **only** in RS-485 class
> (address), in the fault-bit labels, and in the meaning of the first status flag
> ("On/Off" for the heat pump vs "Heating" for the LXi). The wire protocol is identical.
>
> **Important negative result:** *neither heater sim transmits a temperature or a setpoint on
> the wire.* The sims model the heater as a controller-driven slave that reports only
> **state + fault bits**; the setpoint/temperatures live on the controller side (PowerCenter/
> OneTouch), consistent with the project's "3-state at pump+1 in MainStatus" note. See §6.

---

## 1. NetIO imports (both)

`info` confirms both import exactly: `RemoteGetNextAddress, GetMasterMessage, RemoteStatus,
RemoteLogOn, RemoteLogOff` from `NetIO.dll`. The local IAT-jmp thunks (verified):

| Thunk RVA (HeatPump / LXI) | IAT slot | NetIO import |
|----------------------------|----------|--------------|
| `0x16e2` / `0x1606` | `0x42d848` | RemoteGetNextAddress |
| `0x16fa` / `0x161e` | `0x42d84c` | GetMasterMessage |
| `0x16f4` / `0x1618` | `0x42d850` | RemoteStatus |
| `0x16ee` / `0x1612` | `0x42d854` | RemoteLogOn |

(`HeatPump.exe!0x16e2`, `LXI Heater.exe!0x1606`, etc.)

---

## 2. Address (RS-485 class)

`addr = ((class & 0x1f) << 3) | instance`, instance negotiated 0–3 — **CONFIRMED** at
`NetIO.dll!0x1d15` (`and al,0x1f; shl al,3; … or cl,al`, probe loop `cmp ecx,4`).

The class is the immediate `push`ed right before the `RemoteGetNextAddress` call:

| Sim | class arg | site | **base address** | range |
|-----|:---------:|------|:----------------:|-------|
| **HeatPump.exe** | `0x0E` | `push 0xe @0x134a` → `call @0x13a4` | **`0x70`** | `0x70`–`0x73` |
| **LXI Heater.exe** | `0x0D` | `push 0xd @0x1304` → `call @0x1346` | **`0x68`** | `0x68`–`0x6B` |

CONFIRMED. (These are the heater device-class addresses on the Jandy bus — distinct from
how heater *state* also appears inside the controller's MainStatus 0x70 frame.)

---

## 3. Object/slot field layout (per-exe, from the message pump)

The sim's C++ object (`ecx`/`esi` = `this`) holds both the bus state and the DDX-bound UI
controls. HeatPump's object is shifted **+0x10** vs LXI (it has one extra control). Key
offsets, from the message pump + LogOn + DDX:

| Meaning | HeatPump | LXI | Notes |
|---------|:--------:|:---:|-------|
| owner handle (→ LogOn arg) | `+0x1c` | `+0x1c` | |
| RS-485 address byte | `+0xb2` | `+0xa2` | passed as the match-idx to GetMasterMessage/RemoteStatus |
| inbound master-msg buffer | `+0xb3…` | `+0xa3…` | `[0]`=master cmd, `[1]`=control byte |
| inbound control byte (`msg[1]`) | `+0xb4` | `+0xa4` | bit-decoded on cmd `0x0C` |
| RemoteStatus return (rc) | `+0xc4` | `+0xb4` | |
| **outbound 4-byte status frame** | `+0xbd` | `+0xad` | sent on cmd `0x0C` |
| **outbound 3-byte idle frame** | `+0xc1` | `+0xb1` | sent on cmd `0x00`; also the LogOn init buf |

CONFIRMED (`HeatPump.exe!0x1468` pump, `0x1403` LogOn; `LXI Heater.exe!0x140a` pump,
`0x13a5` LogOn).

---

## 4. Master → heater commands (inbound dispatch)

The pump reads the master message into the inbound buffer via `GetMasterMessage`, then
dispatches on `msg[0]`. **Only two command bytes are recognized** (everything else → no-op
return 0):

| master cmd | meaning | heater response |
|:----------:|---------|-----------------|
| `0x00` | poll / keep-alive | send 3-byte **idle** frame (`cmd 0x01`) |
| `0x0C` | **command + status request** | decode `msg[1]` control byte, then send 4-byte **status** frame (`cmd 0x0D`) |

CONFIRMED: `HeatPump.exe!0x14a2` (`mov al,[edi]; test al,al; je idle@0x151f; cmp al,0xc;
jne @0x1539`); `LXI Heater.exe!0x1444` (same shape).

### The `0x0C` control byte (`msg[1]`) — master → heater command bits

On `0x0C` the heater extracts two bits from the master's `msg[1]` (the **enable/command**
field) and stores them into its own UI/state variables:

| `msg[1]` bit | extract | HeatPump → field | LXI → field | meaning |
|:------------:|---------|:----------------:|:-----------:|---------|
| **bit 3** (`& 0x08`) | `shr 3 & 1` | `+0xaa` | `+0x86` | LXI: **"Heat Cmd"** (enable heat). HeatPump: secondary on/off |
| **bit 5** (`& 0x20`) | `shr 5 & 1` | `+0x9e` | n/a | HeatPump: **"Heater/Chiller"** select / 2nd on/off |

CONFIRMED: HeatPump pump `0x14b0` (`movzx eax,[esi+0xb4]; shr eax,5; and 1 →+0x9e` **and**
`shr ecx,3; and 1 →+0xaa`); standalone setters `0x1541` (`+0xb4>>3 →+0xaa`) and `0x155b`
(`+0xb4>>5 →+0x9e`). LXI pump `0x144e` (`mov al,[esi+0xa4]; shr eax,3; and 1 →+0x86`);
standalone setter `0x156e` (`+0xa4>>3 →+0x86`).

> **INTERPRETATION:** the master's `0x0C` command byte is the heater **enable/mode word**.
> The LXi gas heater uses **bit 3 = "Heat Cmd"** (turn heat on). The heat pump additionally
> uses **bit 5** for its "Heater/Chiller" mode select. The sims only *consume* these two
> bits; other bits of `msg[1]` are ignored by the sim (real firmware may use more). No
> setpoint value is parsed from the inbound message — **no setpoint command crosses this wire
> in either sim.**

---

## 5. Heater → master status frames (outbound, via `RemoteStatus`)

Two response shapes, both sent with `sendNow=1` (immediate `CommTx(0x00,…)` to master):

### 5a. Idle frame — reply to master `0x00`
`RemoteStatus(addr, ptr=+0xc1[HP]/+0xb1[LXI], len=3, 1)` —
`HeatPump.exe!0x1531`, `LXI Heater.exe!0x14c0`.

Payload = the LogOn init buffer, **`{0x01, 0x00, 0x00}`** (CONFIRMED):
HeatPump LogOn `0x143d` `mov byte[+0xc1],1`, `0x1430/0x1437` `+0xc2=+0xc3=0`;
LXI LogOn `0x13df` `mov byte[+0xb1],1`, `+0xb2=+0xb3=0`.

| byte | value | meaning |
|:----:|:-----:|---------|
| `[0]` | `0x01` | response/idle command byte |
| `[1]` | `0x00` | reserved |
| `[2]` | `0x00` | reserved |

### 5b. Status frame — reply to master `0x0C`
`RemoteStatus(addr, ptr=+0xbd[HP]/+0xad[LXI], len=4, 1)` —
`HeatPump.exe!0x14e3`, `LXI Heater.exe!0x1472`.

Byte-for-byte (HeatPump offsets `+0xbd…+0xc0`; LXI `+0xad…+0xb0`):

| byte | offset HP / LXI | value | meaning |
|:----:|:---------------:|:------|---------|
| `[0]` | `+0xbd` / `+0xad` | **`0x0D`** | status response command byte (set once in LogOn: `mov byte[+0xbd],0xd` HP `0x1447`; LXI `0x13e9`) |
| `[1]` | `+0xbe` / `+0xae` | **state/flags byte** (bit-packed, see 5c) | |
| `[2]` | `+0xbf` / `+0xaf` | **`0x00`** | **never written** — confirmed untouched in both (always 0; padding/reserved) |
| `[3]` | `+0xc0` / `+0xb0` | **fault bitfield** (see 5d) | |

CONFIRMED. The `[2]` byte being permanently 0 is verified by exhaustive search: the only
reference to `+0xbf` is the `0xbf` *immediate mask* on `+0xbe`, never a write to `+0xbf`
itself; same for LXI `+0xaf`.

### 5c. State/flags byte `[1]` (`+0xbe` HP / `+0xae` LXI)

Built by per-flag setters that OR/AND single bits in. Low 3 bits (`& 0x07`) are forced to 0
right after the inbound decode (`and byte[+0xbe],0xf8` @HP `0x14e8`, LXI `and [+0xae],0xf8`
`0x1477`), so they are spare. The upper bits each track a runtime/UI condition:

**HeatPump** (`HeatPump.exe!0x1575–0x1605`):

| bit | mask | source field | set when |
|:---:|:----:|:------------:|----------|
| 3 | `0x08` | `+0xae` ≠ 0 | (UI state A) `0x1588`/`0x1591` |
| 4 | `0x10` | `+0x7c` ≠ 0 | (UI state B) `0x15aa`/`0x15b3` |
| 5 | `0x20` | `+0xae` ≠ 0 | `0x15cf`/`0x15d8` |
| 6 | `0x40` | `+0xa6` ≠ 0 | `0x15f4`/`0x15fd` |

**LXI** (`LXI Heater.exe!0x1527–0x156d`):

| bit | mask | source field | set when |
|:---:|:----:|:------------:|----------|
| 3 | `0x08` | `+0x7c` ≠ 0 | "Heating" checkbox `0x1531`/`0x1540` |
| 4 | `0x10` | `+0x80` ≠ 0 | `0x1553`/`0x1565` |

> **INTERPRETATION of the 3-state encoding:** The status `[1]` byte carries running-state
> bits. For LXI, **bit 3 (`0x08`) = "Heating"** is the active-heating indicator. The project's
> documented heater 3-state encoding (`0x00`=Off, `0x01`=Heating, `0x03`=Enabled/standby) is
> a *MainStatus 0x70* (controller-side) encoding; these heater **device** sims do **not**
> emit that exact 0x00/0x01/0x03 byte. What they confirm is the *concept*: the heater reports
> a distinct **"Heating" active flag** (LXI bit `0x08`) separate from the master's enable
> command, which is exactly the Heating-vs-Enabled distinction. **Partial confirmation:** the
> Heating state is a real reported bit; the precise 0x01/0x03 packing is a controller-layer
> detail not present in these slave sims. (See §6.)

### 5d. Fault bitfield byte `[3]` (`+0xc0` HP / `+0xb0` LXI)

A true per-bit fault field, packed from DDX-bound checkbox booleans. Bit order and labels
recovered by cross-referencing the **status-builder packing order** with the **DDX
field→control-ID binding** and the **dialog 101 resource captions**.

**HeatPump** — builder `HeatPump.exe!0x1606` packs 7 flags `+0x82,+0x86,+0x8a,+0x8e,+0x92,
+0x96,+0x9a` → byte. DDX binding `HeatPump.exe!0x1238–0x1315`; captions from dialog 101:

| bit | mask | field | control ID | **label (dialog 101)** |
|:---:|:----:|:-----:|:----------:|------------------------|
| 0 | `0x01` | `+0x82` | `0x3F6` | **Shorted Water Sensor** |
| 1 | `0x02` | `+0x86` | `0x3F7` | **Open Water Sensor** |
| 2 | `0x04` | `+0x8a` | `0x3F8` | **High Water Temp** |
| 3 | `0x08` | `+0x8e` | `0x3F9` | **Low Refrig Pressure** |
| 4 | `0x10` | `+0x92` | `0x3FA` | **Hi Refrig Pressure** |
| 5 | `0x20` | `+0x96` | `0x3FC` | **Shorted Coil Sensor** |
| 6 | `0x40` | `+0x9a` | `0x401` | (Open Coil Sensor — UTF-16 string `0x2afe6`) |

(`+0x7c`→`0x3F4`/On-Off and `+0x9e`→`0x3FF`/"Heater/Chiller" are state, not fault; `+0xa6`→
`0x3FB`="J4 Jumper" feeds state bit `0x40`.) CONFIRMED packing; labels CONFIRMED from
resources.

**LXI** — builder `LXI Heater.exe!0x14d0` packs 6 flags `+0x8a,+0x8e,+0x92,+0x96,+0x9a,+0x9e`
→ byte. DDX binding `LXI Heater.exe!0x1242–0x12e8`; captions from dialog 101:

| bit | mask | field | control ID | **label (dialog 101)** |
|:---:|:----:|:-----:|:----------:|------------------------|
| 0 | `0x01` | `+0x8a` | `0x3F6` | **Shorted Water Sensor** |
| 1 | `0x02` | `+0x8e` | `0x3F7` | **Open Water Sensor** |
| 2 | `0x04` | `+0x92` | `0x3F8` | **High Water Temp** |
| 3 | `0x08` | `+0x96` | `0x3F9` | **Fuselink / Field Interlock** |
| 4 | `0x10` | `+0x9a` | `0x3FA` | **High Limit** |
| 5 | `0x20` | `+0x9e` | `0x3FB` | (Shorted Air Temp Sensor — UTF-16 `0x2aeae`) |

(`+0x7c`→`0x3F5`="Heating" → state bit `0x08`; `+0x86`→`0x3FE`="Heat Cmd" = the inbound
enable echo.) CONFIRMED packing; labels CONFIRMED from resources.

---

## 6. Comparison: HeatPump vs LXi, and relation to project notes

| Aspect | HeatPump (heat pump / chiller) | LXi Heater (gas) |
|--------|-------------------------------|------------------|
| RS-485 class / base addr | `0x0E` / **`0x70`** | `0x0D` / **`0x68`** |
| Master cmds | `0x00` (poll), `0x0C` (cmd+status) | identical |
| Idle frame | `{0x01,0x00,0x00}` (3B) | identical |
| Status frame cmd | `0x0D`, 4 bytes | identical |
| Status byte `[2]` | always `0x00` | identical |
| Enable command (`0x0C` `msg[1]`) | **bit 3** + **bit 5** (on/off + mode) | **bit 3 = "Heat Cmd"** only |
| First state flag (`[1]` bit `0x08`) | on/off | **"Heating"** (active) |
| Fault bits (`[3]`) | refrig-pressure / coil-sensor faults | high-limit / fuselink / air-sensor faults |
| **Setpoint on wire?** | **NO** | **NO** |
| **Temperature on wire?** | **NO** | **NO** |

So they are **two different classes** (different addresses) running the **same command
vocabulary and frame format**. They are *not* the same device — a controller addresses a
heat pump at `0x70` and an LXi at `0x68`.

**Setpoint / temperature (units/scaling):** *no setpoint or temperature value is packed by
either heater sim* — exhaustively confirmed (no `imul`/`MulDiv`/temperature scaling anywhere
in either binary; the status frame is `[0x0D][flags][0x00][faultbits]`). The heater is a
slave that the controller turns on/off and that reports back state + faults. The **setpoint
and current temperature are owned by the controller** (PowerCenter / OneTouch), which is why
the project sees heater setpoints in MainStatus / page scrapes, not in a heater-addressed
frame. Any "setpoint scaling" question must be answered against `Pwrcntr.exe` / `Onetouch.exe`
(controller side), not these heater sims.

**3-state (`0x00`/`0x01`/`0x03`) check:** *partially confirmed, with a caveat.* These device
sims confirm the *semantic* split — a separate master **enable command** (`0x0C` bit 3) vs a
heater-reported **"Heating" active flag** (status `[1]` bit `0x08`, literally labelled
"Heating" in LXI) — i.e. enabled-but-idle vs actively-heating are distinct on the bus. But
the specific `0x00`/`0x01`/`0x03` *byte values* are a **controller-side MainStatus 0x70
encoding**, not produced by these heater-device frames. The heater frames use independent
bits, not that packed tri-state byte.

---

## 7. Net for aqualink-automate

- **New addresses for heater devices:** heat-pump class `0x0E` → **`0x70`–0x73**; LXi gas
  class `0x0D` → **`0x68`–0x6B**. (Project device-id table / `jandy_device_types.h`.)
- **Heater device frame (if/when captured):** master polls `0x00`; sends enable as cmd
  **`0x0C`** with the enable in **`msg[1]` bit 3** (LXi "Heat Cmd"), heat-pump also bit 5;
  heater replies cmd **`0x0D`**, 4 bytes `[0x0D][stateflags][0x00][faultbits]`. Idle reply
  cmd `0x01`, 3 bytes `{0x01,0x00,0x00}`.
- **Fault decode tables** (§5d) are ready to use for both heater types — they're true
  per-bit bitfields with the exact labels above.
- **Do not expect a setpoint/temperature in a heater-addressed frame** — it isn't there.
  Setpoints come from the controller (MainStatus / OneTouch pages). This is consistent with
  and reinforces the existing "heater 3-state at pump+1 in MainStatus" project note.

> **CAPTURE-GATED:** the precise meaning of `msg[1]` bits beyond 3/5, the exact controller
> command that drives a *setpoint*, and confirmation that real heater firmware uses the same
> `0x0C`/`0x0D` pair (the sim is the vendor's emulator, very likely faithful, but a live
> capture against a `0x68`/`0x70`-addressed heater would close it) are not resolvable from
> these slave sims alone.
