# UAPI Client.exe — the iAqualink2 "UAPI" host protocol (RE findings)

Target: `C:\Program Files (x86)\Alwin32\UAPI Client.exe` (≈1 MB, x86, MFC debug build,
VS2003-era `Rev S` source tree). Reverse-engineered statically with
`C:\Users\i_che\alwin32-re\alw.py` (pefile + capstone). Citations are `UAPI Client.exe!0xRVA`
unless noted; **VA = 0x400000 + RVA**. Capstone prints VAs (e.g. `0x42b256`), so the RVA is
`VA − 0x400000`.

Status tags: **CONFIRMED** = read directly from code/data; **INFERRED** = deduced, flagged.

---

## 1. What UAPI is — and its transport

**CONFIRMED.** UAPI Client is the *"Zodiac UAPI Test Client"*, **Rev 1.4** (resource strings,
`!0xe7b8e`, `!0xe7c4e`; title-bar `UAPI Client` `!0xeac0a`; copyright "© 2013" `!0xe7c08`). The
embedded VERSIONINFO calls it `PCDOCK MFC Application` / `PCDOCK.EXE` (`!0xe8580`,
`!0xe8684`). Source paths in the binary: `C:\AQUALINK\Rev S\Alwin32 VC++ 5.0\UAPI Client\…`
(`!0xd5810` etc.), PDB `…\UAPI Client\Debug\UAPI Client.pdb` (`!0xf8b20`).

It is **not** a sockets/HTTP/JSON client. Its only protocol import is **`NetIO.dll`** — the same
RS-485 wire-protocol DLL the rest of the Alwin32 suite links
(`info`: imports `CommInit, CommID, CommInUse, CommEnd, CommRxCount, RemoteLogOn, RemoteLogOff,
RemoteStatus, GetMasterMessage`). There is **no WS2_32 / WinSock, no WinHTTP/WinINet** import.

* **Transport = a serial COM port** driven through NetIO. `CommInit` is called with a `"COM%d"`
  label (`!0x3995f` pushes `"COM%d"` `@0x4d73b4`, then `CommInit` `!0x40398c/0x403991`). On
  failure UAPI shows *" Initialization failed on COM%d (%d) "* (`!0xad08`, string `@0x4e1214`).
* **"RS-485 Bus Master Search"** dialog (`!0xe81de`) + *"Searching for communication from '485
  bus master."* (`!0xe8266`) → uses NetIO **`CommID`** (`callsites CommID` → one site at
  `!0x1f0c`). So UAPI passively listens for an Aqualink RS bus master and times out after
  ~30 s ("This search can take up to 30 seconds" `!0xe8356`).

**Conclusion:** UAPI Client is a **PC-side emulator of the iAqualink2 Wi-Fi module**. It logs
onto the RS-485 bus as the iAqualink2 virtual device(s) and exercises the **application-level
"UAPI" status protocol** the controller speaks to its Wi-Fi/cloud gateway. "UAPI" = the
**U**niversal **API** that the iAqualink module consumes — exactly the
`MAINSCR_INFO / ONETOUCH_INFO / AUXILIARY_INF / IPHONE_INFO` family the project cares about.

### Dual-device addressing — 0x33 + 0xA3 (CONFIRMED)

UAPI calls **`RemoteLogOn` for two addresses** (`callsites RemoteLogOn`, setup at `!0x2ac80…`):

| address build (in code)                     | class | instance | address | role |
|---------------------------------------------|:-----:|:--------:|:-------:|------|
| `and al,7; or al,0xA0; …or cl,3` (`!0x2ad3e`)| 0xA0 (IAQ)         | 3 | **0xA3** | iAqualink2 cloud device |
| `and al,7; or al,0x30; …or cl,3` (`!0x2ad83`)| 0x30 (AqualinkTouch)| 3 | **0x33** | rich-status / page side |

Each logs on with the 128-byte init buffer seeded to **`{0x01,0x00,0x00}`** (the device reply
template, §4). This is exactly the project's iAqualink2 dual-device model
(0xA3 heartbeat + 0x33 rich status); UAPI emulates **both**. The address is stored at struct
offset `+0x10B` and the outbound reply buffer at `+0x10C` (used by every `RemoteStatus` call).

---

## 2. Message framing — TLV over the Jandy DLE frame

The Jandy DLE+STX/ETX framing and 8-bit checksum are handled inside `NetIO.dll` (documented in
`alwin32_simulator_protocol.md §2`). UAPI never touches raw bytes; it works with the **already
de-framed payload**: `GetMasterMessage(addr, outBuf)` returns `[cmd][payload…]` (cmd at
`+0xC6`, payload from `+0xC7`), and `RemoteStatus(addr, dataPtr=+0x10C, len=+0x136, sendNow)`
sends `[cmd][payload…]` back to the master (dest 0x00).

**On top of that, the UAPI payload is a TLV (tag-length-ish-value) container** — this is the
new, project-relevant finding. Decoded from the symmetric reader/writer pair:

### Container shape (CONFIRMED)

```
payload = [ cmd ] [ count ] [ record ]*count
record  = [ tag_hi ] [ tag_lo ] [ value? ]      ; tag is 16-bit BIG-ENDIAN
```

* **Record reader** `!0x21a0` (thunk `0x401109`): reads `b0,b1`, assembles
  `tag = (b0<<8)|b1` (`shl …,8; or` — **big-endian**), advances 2. Then calls predicate
  `!0x11b3` (`0x4011b3`) with the tag; **if it returns true it reads one more byte = the value**
  and the record is 3 bytes, else 2. Returns the record length consumed.
* **Record writer** `!0x2374` (thunk `0x401064`): mirror image — emits `tag_hi, tag_lo`, then
  if the predicate says so appends one `value` byte. (`!0x2374`–`!0x2402`.)
* The dispatch loop at `!0x2b294…` reads a leading **count** byte (`+0x64`), then loops `count`
  times reading records and decrementing the counter (`sub dl,1` `!0x2b962`).
* A two-byte value field also exists (reader `!0x1208`→`0x401208`, used for the version tag).

So the value length is **per-tag** (governed by the `0x4011b3` table), not a literal length
byte — closer to "tag + optional fixed-size value" than classic TLV. Most fields are 1 byte; the
version field is 2 bytes.

INFERRED: in practice the **low byte of the tag is the field id** (the dispatchers below index on
`tag & 0xFF`); the high byte is 0 for the observed fields. The 16-bit big-endian read leaves room
for >255 field ids.

---

## 3. The bus command set (CONFIRMED)

The master-message dispatcher is at `!0x2af80…` (the function reached after
`GetMasterMessage`). It special-cases two address-echo commands (`cmp …,0x83`/`cmp …,0xA3`
`!0x2aff6`,`!0x2b00e`) then does a **jump table on `(cmd − 0x30)`**, range 0x30–0x86 (idx table
`@0x42bdab`, jumptable `@0x42bd97`, decoded). Only these have real handlers; everything else →
generic-ACK default.

| cmd  | direction        | handler | meaning |
|-----:|------------------|---------|---------|
| `0x30` | master→UAPI    | `!0x2b0de` | **Poll / keep-alive + online state machine.** Drives the request/subscribe logic (§5); first contact sets "ON LINE" (`@0x4e1290`) and flags `+0x143/+0x13d`. |
| `0x82` | UAPI→master    | builders `!0x2beb7`,`!0x2c076` | **STATUS REQUEST / subscribe** (UAPI asks the controller for info blocks). TLV body lists wanted field tags. |
| `0x83` | master↔UAPI    | parse `!0x2b256`; UAPI build `!0x2c536` | **MAIN-SCREEN status block** (TLV, field tags 0x01–0x1F). The "MAINSCR_INFO" payload. |
| `0x84` | master↔UAPI    | parse `!0x2b256`; UAPI build `!0x2c21a` | **status block, same TLV codec** as 0x83 (handler shared via case 2). |
| `0x85` | UAPI→master    | builder `!0x2c491` | a request variant (TLV). |
| `0x86` | master↔UAPI    | parse `!0x2b9e6` | **AUX / extended status block** (TLV, field tags 0x1E–0x34) — the "AUXILIARY_INF" payload. |
| `0x80` | master→UAPI    | `!0x2b97f` | clears the request/subscribe flags (`+0x145/+0x147/+0x14F = 0`) then plain ACK — a **reset / "stop sending"**. |
| else | master→UAPI     | `!0x2bce2` | generic **ACK** `{0x01,0x00,0x00}` echoing the received cmd byte. |

The reply command byte each UAPI builder stamps into `+0x10C` was read directly:
`0x82` (`!0x2bec3`, `!0x2c07x`), `0x83` (`!0x2c536`), `0x84` (`!0x2c21a`), `0x85` (`!0x2c491`).
The TLV body of a 0x82/0x85 request is appended with the `!0x2374` writer (`push 0x03; push len`
record loops at `!0x2bee0`+).

**Relation to the §6 iAqualink2 WiFi command-name table** (ENQ/SSID/…/MAINSCR_INFO 0x70 …
IPHONE_INFO 0x73, recovered from `NetIO.dll` master logger): those names belong to the master's
*logging vocabulary*; on **this** device-facing bus the same semantics ride command bytes
**0x82–0x86** with the TLV body above. INFERRED mapping by semantics:
`0x83 ≈ MAINSCR_INFO`, `0x86 ≈ AUXILIARY_INF`, `0x82/0x85 ≈ the *_INFO request/poll`. (The
numeric opcodes differ from 0x70–0x73 — do not assume they are equal; treat as two encodings of
the same status set. Reconcile against a capture if it matters.)

---

## 4. The device reply (CONFIRMED)

Every dispatch arm finishes with
`RemoteStatus(addr=+0x10B, data=+0x10C, len=+0x136, sendNow=1)` (`!0x2b1fa`, `!0x2b28c`,
`!0x2b9d9`, `!0x2ba1b`, `!0x2bd0e`). Unless an arm built a 0x82/0x83/0x84/0x85 request, the reply
buffer is prepared by `!0x2be84` (thunk `0x4010a5`):

```
+0x10C = 0x01   +0x10D = 0x00   +0x10E = 0x00     ; payload = {01 00 00}
+0x136 = 3                                         ; length 3
```

i.e. the device-side answer to a poll is the generic **3-byte ACK `01 00 00`** — identical to the
log-on init buffer and to the AqualinkTouch/iAQ ACK seen elsewhere in the suite. The **rich data
flows master→UAPI** (the 0x83/0x84/0x86 blocks UAPI *parses*), with UAPI only ACK-ing and
periodically emitting 0x82/0x85 *requests*.

---

## 5. The MAINSCR / AUX field tables (CONFIRMED decode)

Inside the 0x83/0x84 parser (`!0x2b256`) is an inner **jump table on `tag_lo`**
(idx `@0x42be3a`, jumptable `@0x42be02`, range 1–0x1F, decoded). Each arm decodes one field and
stores it into a UI-bound CString at a struct offset (via `0x456edd` = `CString::operator=`).
The mapping below is read straight from the handlers + the format strings they use; the dialog
labels (`Switch / Pump / Temp / Status / Type / Label / Mode / Config / AUXs / Last AUX`,
resource `!0xe7cca…0xe8086`) name the same fields.

### 0x83 / 0x84 — MAIN-SCREEN block, tags 0x01–0x1F

| tag  | value | decode → display | store off | cite |
|-----:|-------|------------------|:---------:|------|
| `0x01` | 2 bytes | **firmware version** → `"UAPI Rev %d.%02d"` (`@0x4e1298`) | +0xC0 | `!0x2b335` |
| `0x03` | 1 byte | **system/equipment type**: 1=`Pool/Spa`, 2=`Pool Only`, 3=`Dual Equ`, else `???` | +0xAC | `!0x2b42c` |
| `0x04` | string | **label/name** (`Label:` field); also clears flag +0x13D, sets +0x13F | +0x9C | `!0x2b3ca` |
| `0x05` | string | text field | +0xA0 | `!0x2b4b1` |
| `0x06` | string | text field | +0xA4 | `!0x2b4f4` |
| `0x07` | 1 byte | **mode**: 1=`AUTO`, 2=`SERVICE`, 3=`TIMEOUT`, else `???` | +0xA8 | `!0x2b538` |
| `0x09` | 1 byte | **selected/"Last AUX" index** (`%d`); also sets the tracking idx +0x151/+0x153 | +0xB0,+0x98 | `!0x2b5bd` |
| `0x0A` | 1 byte | **state of the selected item**: `OFF`/`ON`, +num if >1 (compared vs sel idx +0x153) | +0x80 | `!0x2b875` |
| `0x0B` | string | text field; clears +0x13F, sets +0x141 | +0x8C | `!0x2b8fa` |
| `0x14` | 1 byte | **temperature unit**: 1=`F`, 2=`C`, else `??` | +0x94 | `!0x2b673` |
| `0x15` | 1 byte | **temperature value**, stored as `value − 0x14` (−20 offset); 0 → `N/A` | +0x88 | `!0x2b6de` |
| `0x1E` | 1 byte | **switch/pump state**: 0=`OFF`, 1=`ON`, 2=`SWITCHING` | +0x84 | `!0x2b751` |
| `0x1F` | 1 byte | per-item state (vs sel idx +0x153): `OFF`/`ON`, +num | +0x80 | `!0x2b7c6` |

Tags 0x02, 0x08, 0x0C–0x13, 0x16–0x1D fall through to "skip/ignore" (case 13 → `!0x2b95f`).

> **Temperature is offset-encoded: actual = wire_byte − 20.** (`sub cl,0x14` `!0x2b716`.) A wire
> byte of 0 is treated as **N/A** (sensor unavailable), matching the project's "raw temp 0 when
> pump off → filter" note. CONFIRMED.

> **Heater three-state vibe:** the switch field 0x1E uses 0/1/2 = Off/On/**Switching** (a
> transitional state), not the project's Off/Heating/Enabled triplet — different field, noted for
> comparison. CONFIRMED values, INFERRED relationship.

### 0x86 — AUX / extended block, tags 0x1E–0x34

Parser `!0x2b9e6` with its own inner jump table on `tag_lo` (idx `@0x42be6d`, jumptable
`@0x42be59`, base 0x1E, range 0x1E–0x34, decoded). Handlers mirror the main block's state codec:

| tag  | value | decode → display | cite |
|-----:|-------|------------------|------|
| `0x1E` | 1 byte | numeric `%d` field; gated by request flag +0x145 | `!0x2bac5`-region |
| `0x1F` | 1 byte | aux state `OFF`/`ON`/`SWITCHING` (`@0x4e1330/34/38`) | `!0x2bb87` |
| `0x33` | 1 byte | numeric `%d` (`@0x4e132c`) | `!0x2bac5` |
| `0x34` | 1 byte | aux state vs sel idx +0x153: `OFF`/`ON`, +num (`@0x4e1344/48/4c`) | `!0x2bb44` |

Tags 0x20–0x32 are skip/ignore (case 4 → `!0x2bca7`). INFERRED: the 0x86 block enumerates the
auxiliary relays (the dialog's `AUXs:` / `Last AUX` panel), each as an `OFF/ON/SWITCHING` +
optional value record.

---

## 6. The online / subscribe state machine (CONFIRMED structure)

On poll cmd `0x30` (`!0x2b0de`) UAPI runs a flag-driven sequence that emits the request commands
in §3, advancing a small state machine on flags at struct offsets `+0x13D,+0x13F,+0x145,+0x147,
+0x14F`:

* `+0x13D` set → send **`0x82`** builder `!0x11d1`→`!0x2beb7` (initial subscribe; appends record
  tags 3,4,5,… via the `!0x2374` writer).
* `+0x13F` set → send **`0x82`** variant `!0x11f4`→`!0x2c076`.
* `+0x14F` set → `!0x11cc`→`!0x2c21a` (**0x84**).
* `+0x145` set → `!0x1203`→`!0x2c491` (**0x85**).
* `+0x147` set → `!0x1050`→`!0x2c536` (**0x83**).
* On first poll while offline it writes `"ON LINE"` to +0x7C (`!0x2b213`) and arms the sequence;
  cmd `0x80` tears it down (clears the flags, `!0x2b97f`).

INFERRED: this is the iAqualink2 module's **handshake/subscription** — after the bus master
starts polling (0x30), the module requests the MAINSCR/AUX info blocks (0x82/0x83/0x84/0x85),
then the master streams them back as 0x83/0x84/0x86 status, which UAPI parses into §5's fields.

---

## 7. Relevance to aqualink-automate (iAQ work)

* **Confirms the dual-device iAqualink2 model** the project already uses: the official Wi-Fi
  emulator logs onto **0x33 (AqualinkTouch) and 0xA3 (IAQ)** simultaneously and ACKs with
  `{01 00 00}` — independent vendor confirmation of `jandy_device_types.h` classes 0x30 & 0xA0
  and the passive-ACK behaviour.
* **A clean, ground-truth field map for the iAqualink2 status payload** (§5): version, system
  type (Pool/Spa…), mode (AUTO/SERVICE/TIMEOUT), temp unit (F/C), **temperature with a −20
  offset and 0=N/A**, switch/pump state (OFF/ON/SWITCHING), and the AUX enumeration. These are
  directly comparable to what the project decodes from MainStatus/AuxStatus and the iAQ pages —
  use them to validate field semantics (esp. the −20 temp offset and the N/A=0 rule).
* **A concrete TLV codec** (`[cmd][count]([tag16-BE][value?])*`) for the 0x82–0x86 command set.
  This is a *different* opcode space from the master's 0x70–0x73 `*_INFO` names — flag this when
  cross-referencing; don't assume 0x83≡0x70 byte-for-byte without a capture.
* **Highest-leverage next step (fixture generation):** point `Pwrcntr.exe` (master) at UAPI
  Client over a virtual COM pair / the app's RFC2217 bridge and `--record-serial` the exchange →
  a `.cap` of the *real* iAqualink2 status protocol (0x82–0x86 TLV) for the replay harness. UAPI
  is the only sim in the suite that speaks the iAqualink2-side status protocol, so this is the
  way to get iAQ fixtures without hardware.

---

## 8. Caveats / honesty

* The **0x82/0x85 request bodies**: I confirmed the command byte and that they TLV-append records
  via `!0x2374`, but did not exhaustively enumerate every tag pushed in each builder — the field
  *parse* side (§5) is the fully-decoded part. INFERRED that the request lists the same tag ids.
* The **16-bit big-endian tag**: the high byte is 0 for every observed field; the wide read is in
  the code but unused range. Treat field id = tag low byte in practice.
* Opcode↔name correspondence to the §6 WiFi table is **INFERRED by semantics**, not proven equal.
* All offsets are into UAPI's per-device struct (the `+0x10B/+0x10C/+0x136` triplet is the
  NetIO slot mirror; `+0x80…+0xC0` are the parsed-field CStrings bound to dialog controls).
