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

### OBS-01 — IAQ (AqualinkTouch 0x33) page-push not driven by the sim for RS models  *(Investigate)*
- **Symptom:** emulating IAQ `0x33` against an RS-8 Combo master, the device receives only
  `IAQMessage_StartUp` — no MainStatus / page frames — so config stays `Unknown` and no equipment is decoded.
- **Impact:** on this sim the OneTouch menu-scrape is the only working data path for RS models; the IAQ path
  yields nothing. (A prior session captured live IAQ pages — `iaq_pages_live.cap` — so page-push DOES work in
  some config; reconcile which models/revisions the master drives via page-push vs menu.)
- **Status:** OPEN (investigate — may be expected per model/revision; relevant to the auto-startup data-method choice).

### OBS-03 — Heater / pool / spa setpoints decode as null on *some* models  *(Investigate)*
- **Symptom:** `pool_setpoint` / `spa_setpoint` are null after a full scrape on **PD-8 Combo** and **RS-8 Combo**,
  but **decode correctly on RS-8 Only** (pool 80 °F / spa 60 °F). So it is model/config-dependent, not a blanket
  failure.
- **Crawl evidence (RS-8 Combo):** the completed crawl visited `SetTemperature` (22) but **not** `SetPoolHeat` (23)
  or `SetSpaHeat` (24) — those sub-pages are where the per-body setpoint value lives. On RS-8 Only the value is
  available from the page that *was* visited. So the gap is likely: on Combo the setpoint sits on a SetPoolHeat/
  SetSpaHeat sub-page that the discovery crawl does not enter (they are value-edit pages, not discovery targets).
- **Next:** confirm whether SetPoolHeat/SetSpaHeat should be discovery targets on Combo, or whether the value can
  be read from the SetTemperature page directly; set a setpoint in the sim and re-scrape to confirm the read path.
- **Status:** OPEN (investigate; not blocking — read path works on SingleBody).

### OBS-04 — Air temperature blank on RS-2/10 Dual  *(Investigate)*
- **Symptom:** air temp reads 72 °F on RS-8 Combo and RS-8 Only but is **blank on RS-2/10 Dual** (same idle sim).
- **Impact:** minor; may be that the dual-equipment model surfaces air temp on a different page/field, or the sim
  simply does not report it for this model. Reconcile against the dual model's status page.
- **Status:** OPEN (investigate; low priority).

### OBS-05 — Service/Timeout mode not published to MQTT  *(Minor / easy follow-up)*
- **Finding:** `EquipmentMode` (Normal/Service/TimeOut, `data_hub.h`) is detected from both the navigation
  layer (`Navigator::HandleSpecialPage`, Service→Failed, Timeout→recoverable wait) and the RSSA OPMODE
  (`SerialAdapter_SCS_OpModes`), and IS exposed over the **WebSocket** (`equipment_mode` field, plus a
  `service_mode` state string) — but is **not** published over **MQTT** (`mqtt_hub.cpp` circulation/config
  serializers omit it).
- **Impact:** a Home-Assistant/MQTT-only consumer can't see when the controller is in Service/Timeout. For a
  smart-home branch this is worth surfacing (a binary_sensor/attribute). Safe additive change.
- **Status:** OPEN (catalogued; non-blocking).

### OBS-06 — SpaFill / SpaDrain circulation modes defined but unimplemented  *(Minor)*
- **Finding:** `CirculationModes` (`circulation.h`) declares `Pool, Spa, SpaFill, SpaDrain, Spillover`, but
  `SerialAdapterDevice::SetCirculationMode` only actions Pool/Spa/Spillover; `SpaFill`/`SpaDrain` hit the
  `default` branch → logs "Invalid circulation mode" and returns `InvalidValue` (an honest rejection, not a
  silent no-op).
- **Impact:** Spa Drain / Spa Fill can't be commanded. On real AquaLink these aren't simple RSSA pump commands
  (they're valve/service operations), so this likely needs protocol research before it can be implemented — not
  a regression. "Where it makes sense" this only applies to Combo/Dual (spa-capable) panels.
- **Status:** OPEN (catalogued; needs protocol basis + design — not a blocking bug).

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

### OBS-07 — Model ground truth exists but is never used to validate discovery  *(Enhancement)*
- **Finding:** `PoolConfigurationDecoder::AuxillaryCount()` / `PowerCenterCount()` are defined but **never
  called**; the `PowerCenters` class (A/B/C/D, caps [7,8,8,8], correct "Aux"/"Aux Bn" naming) is **never
  instantiated**. The OneTouch version processor reads `Configuration()`/`SystemBoard()` only. So the discovered
  aux set is accepted with no cross-check — a model expecting 15 aux that scrapes 12 (a wedged/short crawl) is
  silently accepted.
- **Opportunity:** after the model is decoded, store expected aux/PC count in DataHub; after the equipment
  scrape, compare discovered vs expected and warn/expose a mismatch (catches incomplete scrapes, mis-wired
  panels, and — combined with the BUG-01 fail-fast — repeated model-missing-target failures). Also attribute
  each aux to its power center via the existing `PowerCenters` class.
- **Status:** OPEN (enhancement; ground truth already in-tree).

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
