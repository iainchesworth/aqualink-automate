# Simulator-in-the-loop validation — defect catalogue

Living catalogue of issues/bugs/defects found by driving the app with the **Alwin32**
controller simulator over a com0com virtual serial pair, across controller models and
operating modes. Discovery pass first; fixes tracked per entry.

## Test rig

- com0com pair **COM3 ↔ COM5** (COM6 is a real Prolific USB-serial, unrelated).
- Master: `C:\Program Files (x86)\Alwin32\Pwrcntr.exe`. Model set via
  `C:\Users\i_che\AppData\Local\VirtualStore\Windows\Pwrcntr.INI` →
  `[Options] ControllerType=<decimal> CommPort=3`. The sim auto-opens the port and polls on
  launch (no GUI needed).
- App: `aqualink-automate --serial-port COM5 --jandy-device-type onetouch --jandy-device-id 0x41
  --http-port 8190 --disable-https --record-serial <x>.cap` then query `/api/equipment`.
- Harness: `C:\Users\i_che\alwin32-re\run_model.ps1 -ControllerType <n> -Label <x> -WaitSec <s>`.
- ControllerType decimals: RS-8 Combo `29221`, RS-6 Combo `29222`, RS-4 Combo `29223`,
  RS-8 Only `29224`, RS-2/6 Dual `29227`, RS-2/10 Dual `29234`, RS-2/14 Dual `29235`,
  RS-12 Combo `29230`, RS-16 Combo `29231` (0x722e/2f), RS-12/16 Only `0x7230/31`,
  PD-8 Combo `260808`, PD-8 Only `260809`. (No "spa-only" model — kinds are Combo/Only/Dual.)

## Config / startup validation matrix

| ControllerType | App model | App config | bodies | aux | heaters | pumps | notes |
|---|---|---|---|---|---|---|---|
| 260808 PD-8 Combo | PD-8 Combo | DualBody_SharedEquipment | Pool,Spa | 7 | 3 | 1 | full scrape OK (after wedge) |
| 29221 RS-8 Combo | RS-8 Combo | DualBody_SharedEquipment | Pool,Spa | 7 | 3 | 1 | **full scrape OK after BUG-01 fix** — 28 pages, complete; aux/heaters(Pool/Spa/Solar)/pump(Filter) all Off (sim idle); setpoints null (see OBS-03) |
| 29224 RS-8 Only | RS-8 Only | SingleBody | Pool | 7 | 1 (Solar) | 1 (Filter) | **full scrape OK** — setpoints decode (pool 80°F / spa 60°F); aux all Off |
| 29234 RS-2/10 Dual | RS-2/10 Dual | DualBody_DualEquipment | Pool,Spa | 11 | 2 (Pool/Spa) | 2 (Pool/Spa Pump) | **full scrape OK** — B-side aux (Aux B1–B4) + Extra Aux present; **Aux1=On, Extra Aux=On decoded live** (read-path ✓); air temp blank (OBS-04) |

(Filled in as the sweep proceeds.)

**Scrape timing (RS-8 Combo, post-fix):** a full OneTouch discovery crawl visits 28 pages and
takes ≈140-150 s end-to-end (inherent to menu-scraping ~30 pages over RS-485; not a defect, but
worth noting for any "ready within N s" expectations). 4 targets fail-fast as model-missing on this
panel: `DiagnosticsIAQStatus`, `DiagnosticsIAQRSSI` (iAqualink-only), `Boost`, `SetAquapure` (SWG-only).

---

## Defects

### BUG-01 — OneTouch startup scrape wedges on a model-missing menu target  *(High)*  — **FIXED**
- **Models:** all non-iAqualink / no-SWG panels (PD-8, RS-* without an AqualinkTouch). Surfaced on PD-8 Combo + RS-8 Combo.
- **Symptom:** during the SpiderEngine startup crawl the Navigator repeatedly fails to reach a target whose
  menu item does not exist on this model: cycles of `Unexpected page after navigation - expected
  11(DiagnosticsIAQStatus), got 5(Help)` + `Cursor wrap detected ... (target=11)` + `Recomputed path`. The
  cursor wraps past the missing menu line, the navigator "proceeds" onto a wrong page, recomputes, and repeats —
  ~23–50 cycles per missing page before it gives up.
- **Affected targets on RS-8 Combo:** `DiagnosticsIAQStatus` (11), `DiagnosticsIAQRSSI` (12) — iAqualink-only;
  `Boost` (27), `SetAquapure` (28) — SWG-only. Each wedged the crawl independently.
- **Impact:** startup scrape drastically slowed and starved inside a normal observation window (RS-8 Combo:
  aux/heaters/pumps still 0 at 75 s; the Boost/SetAquapure wedges occur late in the crawl so EquipmentOnOff
  was never reached).
- **Root cause:** the OneTouch menu model registers pages that only exist on iAqualink-equipped / SWG-equipped
  panels. When the target item is physically absent the cursor cannot land on it; the Navigator recomputed up to
  `MAX_RECOMPUTE_COUNT` (50) before failing a target, and the spider only advances after a target fails.
- **Fix (`navigator.{h,cpp}`):** fail-fast after `MAX_STUCK_RECOMPUTES` (3) recomputes for the same TARGET.
  The detector keys on the **target alone**, not on `(actual,target)` — the wrong page the cursor lands on
  *varies* from cycle to cycle (Help → Menu → PasswordSettings → SetTime …), so an `(actual,target)` key reset
  the counter every cycle and let the loop grind to 50. Keying on the target catches both the same-page
  (DiagnosticsIAQStatus → always Help) and varying-page (Boost) variants. Regression test:
  `TestUnreachableTarget_VaryingWrongPages_FailsFast` in `test_navigation_navigator_edgecases.cpp`.
- **Verified (live sim, RS-8 Combo 29221):** all 4 missing targets now log `Target N appears unreachable on
  this model (recomputed 3 times ...) - failing fast`; the crawl **completes** ("Crawl complete - 28 pages
  visited → entering NormalOperation") and decodes the full equipment set: 7 aux / 3 heaters (Pool/Spa/Solar) /
  1 pump (Filter), all states Off (sim idle). Help-loop dropped 23 → 3-4.
- **Status:** FIXED & verified end-to-end.

### OBS-01 — IAQ (AqualinkTouch 0x33) page-push on RS models  *(Investigate)*  — **CORRECTED + documented**
- **Original symptom (was):** emulating IAQ `0x33` against an RS-8 Combo master, the device "receives only
  `IAQMessage_StartUp` — no page frames", so config stays Unknown.
- **CORRECTION — the original observation was a logging-level artifact.** With `--loglevel-messages trace` the
  RS-8 Combo master clearly DOES drive the full AqualinkTouch page protocol to the emulated 0x33: over ~30 s,
  `IAQMessage_Poll` ×150, `PageStart` ×1, `PageButton` ×7, `PageMessage` ×12, `TitleMessage` ×2, `PageEnd` ×2
  (plus `StartUp` ×2). The earlier run only logged the StartUp notification because it ran at `info`. So
  page-push is NOT absent on RS models.
- **The real gap:** the master drives the **page UI protocol (0x23-0x28)** but does **not** emit the binary
  **`MainStatus` (0x70) / `AuxStatus` (0x72)** status messages, and the app's IAQ config/equipment decode keys on
  MainStatus. So with the IAQ path alone the DataHub stays empty on this sim (config Unknown, 0 aux/heaters/
  pumps, no temps) even though the home page is being pushed. The equipment IS present in the pushed page
  content (the home `PageButton`s = Filter Pump / Pool Heat / Aux…, `PageMessage`s = temps) but the app does not
  currently translate the pushed home page into DataHub equipment/config — it expects MainStatus for that.
- **Why this is not blocking:** the **OneTouch menu-scrape is the working, validated data path for RS models**
  (this whole campaign), and the auto-startup coordinator already falls back from PagePush → MenuSpider when
  page-push yields no data. Real iAqualink2 hardware DOES send MainStatus (the project's live captures contain
  0x16/0x70-class status), so this MainStatus-absence may be specific to how the **sim's** RS-Combo master
  drives an AqualinkTouch.
- **Potential enhancement (capture-gated):** decode equipment/config from the pushed **home page**
  (`PageButton`/`PageMessage`) rather than only from MainStatus — this overlaps the existing page-registry
  survey work (`iaq_page_registry`). Gated on confirming, against real RS+AqualinkTouch hardware, whether
  MainStatus is sent (if it is, no change is needed; the sim is simply not representative here).
- **Status:** INVESTIGATED — original claim corrected (page-push works); residual gap (no MainStatus on the sim
  page-push path → IAQ path decodes nothing for RS models) documented as a capture-gated enhancement, not a
  blocking defect.

### OBS-03 — Heater / pool / spa setpoints decode as null on Combo models  *(Investigate)*  — **FIXED**
- **Symptom (was):** `pool_setpoint` / `spa_setpoint` null after a full scrape on PD-8/RS-8 Combo (decoded fine
  on RS-8 Only).
- **Root cause (not what was first suspected):** the `SetTemperature` page IS visited and DOES render the values
  — trace capture shows `Line 2: 'Pool Heat   80`F'`, `Line 3: 'Spa Heat   102`F'`. The failure was the
  `TemperatureStringConverter` regex: it required a **single-token** area label and a **1-2 digit** value, so the
  two-word "Pool Heat"/"Spa Heat" labels AND the 3-digit spa setpoint (102) both failed to parse. (Spa *water*
  temps of 100 °F+ were latently unparseable for the same reason.)
- **Fix (commit 746bebb):** widened the converter regex to accept multi-word area labels + 1-3 digit values
  (anchored, so trailing junk still rejected); `PageProcessor_SetTemperature` now parses setpoints by area label
  rather than fixed lines. Unit tests + a OneTouch page test added.
- **Verified live:** RS-8 Combo now reports pool_setpoint 80 °F, spa_setpoint 102 °F.
- **Status:** FIXED.

### OBS-04 — Air temperature blank on RS-2/10 Dual  *(Investigate)*  — **FIXED**
- **Symptom (was):** air temp read 72 °F on RS-8 Combo/Only but was blank on RS-2/10 Dual.
- **Root cause:** `PageProcessor_System` read air temp from a fixed **line 6**, but the dual-equipment home page
  lists both Pool Pump and Spa Pump, pushing the air-temp line down to **line 7** (confirmed via trace capture).
- **Fix (commit b5f6609):** parse the home page by scanning all lines and routing each temperature by its area
  label ("Air"/"Pool"/"Spa") rather than a fixed line. Regression tests cover both layouts.
- **Verified live:** RS-2/10 Dual now reports air = 72 °F.
- **Status:** FIXED.

### OBS-05 — Service/Timeout mode not published to MQTT  *(Minor)*  — **FIXED**
- **Finding (was):** `EquipmentMode` (Normal/Service/TimeOut) was exposed over the WebSocket but not MQTT.
- **Fix (commit c49dbf7):** added `equipment_mode` to the `pool/configuration` MQTT payload
  (`MqttHub::SerializeConfiguration`) and an "Equipment Mode" Home Assistant sensor (new
  `HomeAssistantDiscovery::ConfigurationTopic`). Integration test asserts the field is published.
- **Status:** FIXED.

### OBS-06 — SpaFill / SpaDrain circulation modes defined but unimplemented  *(Minor)*  — **RESOLVED**
- **Finding:** `CirculationModes` declares `Pool, Spa, SpaFill, SpaDrain, Spillover`. SpaFill/SpaDrain are used
  only in the defensive `SpaMode()` logic; **nothing decodes them from the wire and the command path rejected
  them** via the `default` branch (generic `InvalidValue`).
- **Assessment:** Spa Fill / Spa Drain are valve-sequencing / service operations — the Jandy RS protocol has no
  single command for them (the RS Serial Adapter exposes only SPA and SPILLOVER pump commands). They are not
  remotely commandable, so rejection is correct by design.
- **Resolution (commit 0001af3):** handle SpaFill/SpaDrain explicitly in `SetCirculationMode`, returning
  `NotSupported` (a well-formed request this controller cannot perform) with a clear log, rather than the generic
  `InvalidValue`. Tests assert Pool/Spa/Spillover=Accepted, SpaFill/SpaDrain=NotSupported.
- **Status:** RESOLVED (documented as protocol-gated; rejection now explicit + correct).

---

## Aux-count & power-center validation (vs `jandy_pool_configuration_decoder` ground truth)

The decoder already holds the per-model ground truth as `{config, board, AuxCount, PowerCenterCount}`
(`jandy_pool_configuration_decoder.cpp`). Scraped equipment cross-checks **exactly**:

| Model | Decoder: aux / PC | Scraped aux (by power center) | Match |
|---|---|---|---|
| RS-8 Combo | 7 / 1 | Aux1–7 (A=7) | ✅ |
| RS-8 Only | 7 / 1 | Aux1–7 (A=7) | ✅ |
| RS-16 Combo | 15 / 2 | Aux1–7 (A=7) + Aux B1–B8 (B=8) | ✅ |
| RS-2/10 Dual | 10 / 2 | Aux1–6 (A=6) + Aux B1–B4 (B=4) **+ Extra Aux** | ✅ (10 regular; Extra Aux is a separate special relay) |

- **Power-center caps confirmed live:** A=7, B=8 (RS-16 Combo) — matches the orphaned `PowerCenters` class
  caps `[7,8,8,8]`. Dual splits relays across bodies so its A-side is smaller (A=6 on RS-2/10).
- **Sim power-center ceiling = 2:** the Alwin32 `ControllerType` set tops out at RS-12/16 Combo and RS-2/14
  Dual (2 PC). 3–4 PC models (RS-24/32, RS-2/22/30) exist in the decoder but have **no sim ControllerType**, so
  PC C/D can only be validated by unit test against the decoder table, not live.

### OBS-07 — Model ground truth exists but is never used to validate discovery  *(Enhancement)*  — **IMPLEMENTED**
- **Finding (was):** `PoolConfigurationDecoder::AuxillaryCount()` / `PowerCenterCount()` were defined but never
  called; the `PowerCenters` class was never instantiated; the discovered aux set was accepted with no cross-check.
- **Implemented:**
  - Refactored `PowerCenters` from a (dual-model-incorrect) count allocator into an **attribution container**;
    membership derives from the scraped aux id via `PowerCenterForAuxId` (id ranges 0x01-07=A, 08-0F=B, 10-17=C,
    18-1F=D; ExtraAux belongs to none).
  - DataHub now stores the model's expected aux/PC count (set when the version/StartUp page is decoded) and an
    `EquipmentValidation` result. `ValidateDiscoveredEquipment` (pure, unit-tested) cross-checks discovered aux
    relays (numbered + DIP-reconfigured cleaner/spillover/sprinkler) and power-centre span against the model;
    the OneTouch device runs it at scrape completion / watchdog abort. Exposed via `GET /api/equipment`
    (`configuration.validation`).
  - `MenuSurveyResult` reports crawl health: pages reached, whether the core Equipment ON/OFF page was reached,
    and failed pages split into expected-absent (capability-gated iAqualink/SWG pages via
    `OneTouchPageCapabilityRequirement`) vs notable. Exposed via `GET /api/diagnostics/emulated-devices`
    (`menu_survey`).
- **Verified live:** RS-16 Combo → 15 aux / 2 PC validated clean; RS-8 Combo → 7 aux / 1 PC clean, menu survey
  24 pages reached + 4 capability-gated pages classified expected-absent + 0 notable. Unit tests cover 1-4 power
  centers (RS-24/32 decoder-only — no sim ControllerType), DIP-reconfigured relays, incomplete scrapes, stray
  out-of-range aux, ExtraAux exclusion, and the page capability classifier.
- **Status:** IMPLEMENTED (commits 66b2028 validation/attribution, a1bc8fc menu survey).

### OBS-08 — Remote power center (RemAux) / Dual Spa Switch / Spa Link  *(RE done; spaside recognised; full decode PAUSED)*

**Device mapping (user-confirmed):** `Remaux.exe` = remote auxiliary power center; **`2x4rem.exe` = "Dual Spa
Switch"** (2×4 spaside remote); **`8button.exe` = "Spa Link"** (8-button spaside remote).

#### Reverse engineering — COMPLETE & documented
- **RemAux** (`docs/alwin32/lpc4-remaux.md`): class 5 → `0x28-0x2B`. Inbound `0x00` poll / `0x02` 6-byte bulk
  relay image / `0x08` set-relay-16bit / `0x09` set-relay-8bit. Outbound `[0x01][relay_bitmask][0x00]` (len 3).
- **Dual Spa Switch + Spa Link** (`docs/alwin32/spaside-remotes.md`): both use a **fixed** hard-coded address
  (no RemoteGetNextAddress). 2x4rem `0x10` (class 0x02), 8button `0x20` (class 0x04). Inbound = a 6-byte
  LED/indicator image at cmd **`0x02` on BOTH** (the 8-button additionally takes a `0x03` text string). Outbound
  = `[0x01][0x00][button_index]` (len 3); the 8-button exposes 9 keys. Button-index↔physical-function map and the
  full LED-byte→indicator map still want a live capture. **Cross-checked with Ghidra decompilation** — which
  corrected the Spa Link command bytes (static disasm had read LED=0x01/text=0x02) and confirmed the RemAux
  dispatch/relay-setter/bitmask functions exactly.

#### Done (commit c230a87)
- Added the missing **Dual Spa Switch device class** (class 0x02 → `0x10-0x13`) to `jandy_device_types.h`.
- Routed **Dual Spa Switch (0x10)** and **Spa Link (SpaRemote 0x20)** to the `KeypadDevice` handler in
  `IdentifyAndAddDevice`, so both spaside remotes are now **recognised supported equipment** (they appear in the
  device inventory instead of logging "Device class not supported"). Unit tests: class resolution +
  `InstanceAddressesForClass` + recognition-creates-a-device.
- **Not done:** a RemAux (`RemotePowerCenter` 0x28) handler — it is not a keypad, so it still falls through to
  "unsupported"; and the full button/LED/relay **wire-level decode** for all three.

#### Why full wire-decode is blocked (the architecture finding)
These three devices reuse **generic command bytes** that the app's *global* command→message factory already
owns, and a device→master frame carries **no source id** (`JandyMessage` exposes only `Destination()`):
- Their device→master STATUS (RemAux relay bitmask, spaside button press) is an **`Ack` (cmd `0x01`)** — the
  same command every device on the bus uses to acknowledge a poll. Today device→master frames are attributed
  only when their command is *unique* (e.g. AquaRite PPM `0x16` is registered unfiltered for exactly this
  reason — see `aquarite_device.cpp`). A generic `0x01` Ack cannot be attributed to RemAux vs a spaside remote
  vs anything else.
- Their master→device COMMANDS (RemAux set-relay `0x08`/`0x09`, the LED image `0x01`/`0x02`) **collide** with
  existing global ids (`Status`, `PDA_Highlight`, `PDA_Clear`, `Ack`). A frame to `0x28`/cmd `0x08` is parsed as
  `PDA_Highlight`, not a RemAux-SetRelay.

So clean decode needs **two** architectural pieces, neither a routine device-add:
1. **Poll-source correlation** — have the protocol/generator track the last-polled non-master address and stamp
   the following Ack with it, so a device can claim "this `0x01` came from me". Moderate change, but it sits on
   the **high-volume Ack hot path** (every device acks every poll). This alone unlocks the device→master decode:
   **RemAux relay bitmask** + **spaside button presses** (the bulk of the useful data).
2. **Per-destination message dispatch** — interpret a frame by (destination-class, command) rather than command
   alone, so `0x28`/`0x08` means RemAux-SetRelay. Deeper change to the factory/generator core. Needed only for
   the LED state and the master-side set-relay commands (the latter is redundant with the status bitmask).

#### Validation + value caveats
- **Sim-unvalidatable:** all three are blocked from a live com0com capture — the vendor equipment sims share
  NetIO's in-memory net table and **deadlock a COM-mode master** (the PowerCenter goes "Not Responding"). Any
  handler would be **synthetic-frame-validated only** until real hardware or a non-deadlocking capture path
  exists. (Sim `ControllerType` also caps at 2 power centers, so RemAux is the only way to exercise PC C/D and
  it is exactly the deadlocking path.)
- **Modest marginal value:** the equipment STATE these devices carry (aux relay on/off, spa-mode) is already
  obtained by the app via the controller scrape (OneTouch/IAQ/RSSA) and the OBS-07 aux/power-center validation.
  The unique value would be (a) recognising the physical accessory on the bus (done for the spaside pair), and
  (b) seeing *which* spaside button a user pressed / a standalone RemAux's relay states without a controller
  scrape.

#### To resume
- **If poll-source correlation is built:** register an unfiltered `JandyMessage_Ack` slot per device that checks
  the correlated source id; decode the RemAux bitmask (`AckType()` byte = relay mask, bit N-1 = relay N) into
  aux states, and the spaside `Command()` byte = button index. Add a minimal `RemotePowerCenterDevice`.
- **If per-destination dispatch is built:** add RemAux SetRelay (`0x08`/`0x09`) + spaside LED-image decoders
  keyed on (class, command).
- Both validated via `MockReplayHarness` with synthetic RemAux/spaside frames built per the two RE docs.

**Status:** RE COMPLETE; spaside remotes RECOGNISED; full wire-decode + RemAux handler **PAUSED** pending the
architecture decision above (and ideally real-hardware/capture validation).

---

## Operating modes — code-level handling (live mode-driving is GUI-gated)

Driving Service mode / valve modes on the sim requires `Simio.exe`/iodll DIPs (GUI; e.g. S1 bit 0x80 =
Spillover), so headless live coverage is limited this campaign. Code-level assessment of whether each mode is
*handled* (per model, where it makes sense):

| Mode | Handled? | Notes |
|---|---|---|
| Service mode | ✅ full | nav-blocks (→Failed), `DataHub::Mode`, RSSA OPMODE, WebSocket-exposed; not commandable (correct — it's a physical switch). MQTT gap = OBS-05 |
| Timeout mode | ✅ full | nav-blocks but recoverable (waits for clear); same state/exposure |
| Pool / Spa (valve) | ✅ full | `CirculationModes::Pool/Spa`, body `IsActive()`, `SpaMode()`; controllable via RSSA |
| Spillover | ✅ full | aux trait + RSSA `SPILLOVER` pump command + status decode + MQTT |
| Spa Drain / Spa Fill | ⚠️ partial | enum members exist; `SetCirculationMode` returns `InvalidValue` (OBS-06) |
| Heater (Off/Heating/Enabled) | ✅ full | tri-state per pool/spa/solar; `HeaterStatuses` + `HeaterOperatingModes` |

---

## Validated (no defect)

- **Config classification** from the scraped model string is correct across all three kinds:
  Combo → `DualBody_SharedEquipment` (Pool+Spa); Only → `SingleBody` (Pool);
  Dual → `DualBody_DualEquipment` (Pool+Spa, separate equipment). (PoolConfigurationDecoder on the panel type.)
- **Equipment set per kind** (post-BUG-01-fix, full crawl): Combo 7 aux / 3 heaters / 1 pump;
  Only 7 aux / 1 heater / 1 pump; Dual 11 aux (incl. B-side Aux B1–B4 + Extra Aux) / 2 heaters / 2 pumps.
- **Model/version scrape**: model number, type and `REV T.0.1` read correctly from the live master.
- **Live READ-PATH** ✓: the app reflects the master's equipment state. Static: RS-2/10 Dual shows
  `Aux1=On, Extra Aux=On` decoded from the sim's idle defaults. Dynamic: after a write-path toggle the app
  re-scraped and detected the `Off→On` (then `On→Off`) state change on Aux1 (RS-8 Only).
- **Live WRITE-PATH** ✓ (RS-8 Only, OneTouch path): `POST /api/equipment/buttons/{uuid}` → CommandDispatcher
  → OneTouchDevice menu navigation (`Queued toggle → Beginning toggle navigation → Toggle completed`) → the
  Pwrcntr master honours the keypress and toggles the relay → app re-scrape confirms. Verified **bidirectional**
  (Aux1 Off→On→Off). Confirms the OneTouch DeviceActuator is wired end-to-end against a live master.
- **Live link** is stable and headless (no Pwrcntr deadlock observed this campaign).

## Still to cover

- Larger models (12/16, RS-4/6) — expected to behave as their kind (Combo/Only/Dual); not re-run.
- Setpoint write-path: app SetpointController → master (only aux toggle exercised live so far).
- Chlorinator path on a model WITH an SWG (sim chlorinator sim deadlocks a COM master — see RE notes; needs
  the app-emulated path or a fixture).
- Operating modes per model where applicable: Service mode, valve modes (Spa Drain / Spillover / Spa Mode) —
  these are driven by `Simio.exe`/iodll DIPs (GUI), so headless coverage is limited; see the Operating-modes
  section below for the code-level handling assessment.
