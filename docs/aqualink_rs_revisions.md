# Jandy AquaLink RS — Software / PPD / CPU Revision History

Reference for the firmware-revision feature gates the emulation start-up coordinator uses
(`src/jandy/startup/jandy_revision_capabilities.{h,cpp}`). The panel reports its **software
revision** as e.g. `REV T.0.1`; the **leading letter is the major revision** and is what gates
protocol/device support. Double-letter / suffixed revisions (`GG`, `HH`, `HH 232`, `II`, `JJ`,
`MM`, `MMM`) are corrections or minor additions on their major letter.

> Transcribed from the TroubleFreePool wiki "Jandy Aqualink RS" page; further detail in the
> official *AquaLink RS Software and PCB Revisions, Nov. 2011*. Sources at the bottom.

## PCB ↔ PPD-chip socket compatibility (PPD era)

The PPD **chip** revision is a *separate* scheme from the software revision letter — a hardware
fit constraint, not a protocol capability (so it is **not** modelled in code):

- PPD chip rev **C–HH** → operates with any PCB that has a **44-pin** socket.
- PPD chip rev **I / II** → only a **Revision I PCB** with a 44-pin socket.
- PPD chip rev **J or newer** → only a **52-pin** socket (second-generation PCB).

## PPD-era software revisions (Rev A–MMM)

| Rev | Year | What changed |
|-----|------|--------------|
| A | 1994 | Alpha testing |
| B | 1995 | Beta testing |
| C | 1996 | First production AquaLink RS |
| D | 1996 | Cleaner JVA socket → auxiliary via dip switch 7 |
| E | 1996 | Fix: programmed ON times not recognised after freeze protection activates |
| F | 1996 | RS 2/6 sharing-a-heater support; light dimming on RS 4 |
| G | 1997 | **First SpaLink RS support**; Cleaner JVA assignable without dip 7 |
| GG | 1997 | (bug) Cancel button locks up system |
| GGG | 1997 | Correction to GG |
| H | 1998 | **First LX / RS-485 connection**; ROM expanded to 128K; SpaLink reads pool temp, OFF when system off |
| HH | 1998 | Fix: time calibration problem with Rev H |
| HH 232 | 1998 | **Support for RS-232 Serial Adapter** |
| **I** | **2000** | **First OneTouch support; includes (RS-485) serial adapter support.** (Rev I runs only in a Rev I PCB) |
| II | 2000 | Fix: auxiliaries assigned to freeze on OneTouch |
| J | 2002 | Change to 52-PIN PPD + new socket on the AquaLink RS PCB |
| JJ | 2002 | Fix: auxiliaries assigned to freeze on OneTouch |
| K | 2003 | FlowLink support; manual-on items reactivate after Auto/Service/Time-Out |
| L | 2004 | Color Light Control, **PC Docking**, **Laminar Light Pulse Control (LPC4)**, Chiller/Heat-Pump, Run-Time; communicates directly with **AquaPure** chlorine generator |
| M | 2006 | AquaPalm wireless remote; change AquaPure chlorine output (AquaPure rev 11230A05+); assign OneTouch buttons to SpaLink RS buttons |
| MM | 2006 | Fix: entering light labels |
| MMM | 2006 | Fix: daylight-savings AquaPalm line jump; AquaPalm light-dimming adjust |

## CPU-era software revisions (Rev N+)

Rev N is a **complete PCB redesign** — the operating chip is no longer a PPD, it is a **CPU board**.

| Rev | Year | What changed |
|-----|------|--------------|
| **N** | **2007** | **PPD → CPU board.** RS-485 to a Jandy AE Heat Pump &/or LXi Gas Heater |
| **O** | **2008** | **Variable-Speed pump communication** |
| **P** | **2009** | **ChemLink, LM3, AutoClear Plus & DuoClear** |
| **Q** | **2010** | **First Touch Screen panel support** (AquaLink Touch — the `0x30–0x33` page protocol) |
| **R** | **2012** | **Internet control** via smartphone or web (iAquaLink — the `0xa3` interface) |
| T.2 | 2013 | — (the sim reports `REV T.0.1`) |
| V | — | Rev V and earlier (pre-mid-2022): up to **4 VS pumps** using pump **dip switches 3-4** |
| **W** | **2022** | Rev W and later (post-mid-2022): up to **16 VS pumps** via a preassigned **PUMP ADDRESS** |
| Xg | — | unknown |
| **Y** | — | Jandy **Infinite WaterColors** LED light controller requires Rev Y+ |
| Yg | 2024 | 24 virtual auxiliary assignments |

## How the coordinator uses this

`DeriveRevisionCapabilities("REV T.0.1")` → Rev **T** (≥ Q) ⇒ `serial_adapter_support`,
`onetouch_support`, `cpu_board`, `variable_speed_pumps`, `chemlink_chlorinators`, `aqualink_touch`,
`iaqualink_cloud` (not yet `addressed_vs_pumps` (W) or `infinite_watercolors` (Y)). The planner then:

- **Classifies early.** A touch-capable revision (Q+) lets the coordinator pick `PagePush` from the
  revision alone (sourced via the SerialAdapter) without waiting out the master's probe cycle.
- **Cross-checks the live bus.** The observed probe set is authoritative for the *method*; the
  revision is a sanity check. The genuine contradictions are flagged
  (`StartupPlan::revision_consistent == false`): **AqualinkTouch probed but pre-Rev-Q**, or
  **OneTouch probed but pre-Rev-I**. The converse (a capable revision whose UI simply isn't
  installed/probed) is normal, not a conflict.
- **Sets peripheral expectations.** `variable_speed_pumps` (O+) → expect ePump traffic;
  `chemlink_chlorinators` (P+) → expect those chlorinator messages; `addressed_vs_pumps` (W+) →
  pumps carry a preassigned address (vs the ≤4 dip-switch regime of O–V); etc.

Note the foundational gate at **Rev I (2000)**: OneTouch *and* the RS-485 RS Serial Adapter the app
emulates as its detector both arrive there — a pre-I PPD panel supports neither, so the coordinator's
"emulate a SerialAdapter first" assumption only holds for Rev I and later.

The sim's `PD-8 Combo` reports `REV T.0.1`, i.e. Rev T — touch-capable, which is exactly why it
drives the page protocol when we emulate an AqualinkTouch at `0x33`.

## Sources

- [Jandy Aqualink RS — TroubleFreePool wiki](https://www.troublefreepool.com/wiki/index.php?title=Jandy_Aqualink_RS)
- [Jandy Aqualink PPD/CPU Revisions — TroubleFreePool thread](https://www.troublefreepool.com/threads/jandy-aqualink-ppd-cpu-revisions.87511/)
- [AquaLink RS Software and PCB Revisions (Nov 2011) — PDF](https://www.poolsupplyunlimited.com/Products/Manuals/Sub53_2012329101223.pdf)
