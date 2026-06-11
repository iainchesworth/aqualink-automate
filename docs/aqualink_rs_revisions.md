# Jandy AquaLink RS — Software / PPD / CPU Revision History

Reference for the firmware-revision feature gates the emulation start-up coordinator uses
(`src/jandy/startup/jandy_revision_capabilities.{h,cpp}`). The panel reports its **software
revision** as e.g. `REV T.0.1`; the leading letter is the major revision and is what gates
protocol/device support.

> Compiled from the TroubleFreePool wiki, the official "AquaLink RS Software and PCB Revisions"
> sheet, and the "Jandy Aqualink PPD/CPU Revisions" thread (sources below). Treat exact dates as
> approximate; the **ordering of feature gates** is what the coordinator relies on.

## Two architectures: PPD then CPU

- **PPD era (Rev A–M):** the controller is a *Programmable Peripheral Device*. PPD **chip**
  revisions `C`–`HH` fit a 44-pin socket; chip rev `J`+ needs a 52-pin socket (2nd-gen PCB).
  (This chip-rev/socket scheme is a hardware-fit concern, separate from the software revision
  letter and not modelled in code.)
- **CPU era (Rev N+):** Rev N (2007) is a complete PCB redesign — the PPD is replaced by a
  **CPU board**. All the modern protocol/device support arrives here.

## Software revision → feature gate

| Rev | ~Year | Arch | What it added (gate) |
|-----|-------|------|----------------------|
| A   | 1994  | PPD  | Alpha testing |
| B   | 1995  | PPD  | Beta testing |
| C   | 1996  | PPD  | First production AquaLink RS |
| D   | 1996  | PPD  | Cleaner JVA socket → aux via dip switch 7 |
| G   | 1997  | PPD  | SpaLink RS; Cleaner JVA without dip 7 |
| L   | —     | PPD  | Color light control, PC Docking, **LPC4** laminar pulse, chiller/heat-pump, run-time; direct **AquaPure** chlorinator comms |
| M   | 2006  | PPD  | AquaPalm wireless remote; change AquaPure output; OneTouch→SpaLink button mapping |
| **N** | **2007** | **CPU** | **PPD → CPU board** (complete redesign); RS-485 to AE Heat Pump / LXi Gas Heater |
| **O** | **2008** | CPU | **Variable-Speed pump communication** |
| **P** | **2009** | CPU | **ChemLink, LM3, AutoClear Plus & DuoClear** chlorinators |
| **Q** | **2010** | CPU | **Touch Screen panel — AquaLink Touch** (the `0x30–0x33` page protocol) |
| **R** | **2012** | CPU | **iAquaLink** internet/cloud control (the `0xa3` interface) |
| W   | 2022  | CPU  | up to **16 address-assigned** Variable-Speed pumps (preassigned PUMP ADDRESS) |
| Y   | —     | CPU  | Jandy **Infinite WaterColors** LED light controller |

## How the coordinator uses this

`DeriveRevisionCapabilities("REV T.0.1")` → Rev **T** (≥ Q) ⇒ `cpu_board`, `variable_speed_pumps`,
`chemlink_chlorinators`, `aqualink_touch`, `iaqualink_cloud` (not yet `addressed_vs_pumps` (W) or
`infinite_watercolors` (Y)). The planner then:

- **Classifies early.** A touch-capable revision (Q+) lets the coordinator pick `PagePush` from the
  revision alone (sourced via the SerialAdapter) without waiting out the master's probe cycle.
- **Cross-checks the live bus.** The observed probe set is authoritative for the *method*; the
  revision is a sanity check. The one true contradiction — the master probes the AqualinkTouch
  range while the revision predates Rev Q — is flagged (`StartupPlan::revision_consistent == false`).
  The converse (Q+ but a OneTouch UI installed, so no touch probed) is normal, not a conflict.
- **Sets peripheral expectations.** `variable_speed_pumps` (O+) → expect ePump traffic;
  `chemlink_chlorinators` (P+) → expect those chlorinator messages; etc.

The sim's `PD-8 Combo` reports `REV T.0.1`, i.e. Rev T — touch-capable, which is exactly why it
drives the page protocol when we emulate an AqualinkTouch at `0x33`.

## Sources

- [Jandy Aqualink RS — TroubleFreePool wiki](https://www.troublefreepool.com/wiki/index.php?title=Jandy_Aqualink_RS)
- [Jandy Aqualink PPD/CPU Revisions — TroubleFreePool thread](https://www.troublefreepool.com/threads/jandy-aqualink-ppd-cpu-revisions.87511/)
- [AquaLink RS Software and PCB Revisions (Nov 2011) — PDF](https://www.poolsupplyunlimited.com/Products/Manuals/Sub53_2012329101223.pdf)
