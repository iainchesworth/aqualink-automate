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
| 29224 RS-8 Only | RS-8 Only | SingleBody | Pool | — | — | — | config ✓ (equipment not re-checked) |
| 29221 RS-8 Combo | RS-8 Combo | DualBody_SharedEquipment | Pool,Spa | 7 | 3 | 1 | **full scrape OK after BUG-01 fix** — 28 pages, complete; aux/heaters/pumps all Off (sim idle) |
| 29234 RS-2/10 Dual | _pending_ | | | | | | |

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

### OBS-03 — Heater / pool / spa setpoints decode as null  *(Investigate)*
- **Symptom:** after a full PD-8 Combo scrape (model + 7 aux + 3 heaters + 1 pump + air temp), `pool_setpoint`
  and `spa_setpoint` are still null.
- **Impact:** unclear whether the Set Temperature page is reached during the scrape, or whether the sim simply
  reports no setpoint (e.g. with the heater off / no body temp because pumps are off → sensor unavailable).
- **Next:** confirm the Set Temp page is visited in the scrape log; set a setpoint in the sim and re-scrape.
- **Status:** OPEN (investigate).

---

## Validated (no defect)

- **Config classification** from the scraped model string is correct across models: Combo →
  `DualBody_SharedEquipment` (Pool+Spa); Only → `SingleBody` (Pool). (PoolConfigurationDecoder on the panel type.)
- **Model/version scrape**: model number, type and `REV T.0.1` read correctly from the live master.
- **Air temperature** scrapes correctly (72°F).
- **Live link** is stable and headless (no Pwrcntr deadlock observed this campaign).

## Still to cover

- Equipment set per remaining model (Dual, 12/16, RS-4/6).
- Read-path operation: drive sim equipment state (Simio.exe) → app reflects (pool/spa pump on, etc.).
- Write-path actuation: app commands (setpoint / aux toggle / chlorinator) → master changes → re-scrape confirms.
- Operating modes per model where applicable: Service mode, valve modes (Spa Drain / Spillover / Spa Mode).
