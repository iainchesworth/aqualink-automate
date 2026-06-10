# A simulator-driven testing ecosystem for Aqualink-Automate

Goal: use the official Jandy *Alwin32* simulator suite (and the protocol we recovered from it)
to produce **`.cap` replay fixtures** that drive the app's existing record/replay harness, so we
can test decode/behaviour **without hardware** — and across controller models/configs that are
hard to obtain physically.

There are two complementary fixture sources. Use both.

```
   ┌─────────────────────────┐         ┌──────────────────────────────────┐
   │ (A) GENERATED fixtures  │         │ (B) LIVE-CAPTURED fixtures        │
   │ from the decoded formats│         │ from the running simulators       │
   │  generate_fixtures.py   │         │  Pwrcntr.exe + device sims +      │
   │  → deterministic, exact │         │  --record-serial                  │
   │  → fast, version-able   │         │  → high-fidelity, real sequences  │
   └────────────┬────────────┘         └────────────────┬─────────────────┘
                │                                        │
                └──────────────►  test/fixtures/*.cap  ◄─┘
                                         │
                          MockReplayHarness / LoadFixture
                                         │
                                 behaviour assertions
```

---

## A. Generated fixtures (deterministic, in-repo)

`test/fixtures/alwin32/generate_fixtures.py` emits `.cap` files in the app's recorder format
(see [docs/RECORD_REPLAY.md](../RECORD_REPLAY.md)) directly from the per-device command/payload
tables in [docs/alwin32/](.). Frame encoding (Jandy DLE/STX + 8-bit-sum checksum + DLE-stuffing,
and the Pentair `0xA5` variant) is implemented once and **self-validated**:

```
python test/fixtures/alwin32/generate_fixtures.py --selftest   # reproduces sample_session.cap
python test/fixtures/alwin32/generate_fixtures.py              # writes the .cap scenarios
```

`--selftest` regenerates the three frames of the committed `test/fixtures/sample_session.cap`
**byte-for-byte**, which is the same cross-check the repo's e2e test applies against
`MessageBuilder` — so a generated fixture is guaranteed to frame exactly as the app expects, and
the generator can't silently drift.

Scenarios shipped (extend `SCENARIOS` to add more):

| fixture | exercises |
|---------|-----------|
| `chlorinator.cap` | AquaRite `0x14`/`0x11`/`0x16` → chlorinator in DataHub (mirrors `sample_session.cap`) |
| `epump.cap` | Jandy ePump `'D'` set-speed (RPM×4 LE) + poll |
| `heater.cap` | LX heater `0x0C` enable+setpoints, `0x0D` status (Heating bit `0x08`) |
| `iaq_page.cap` | IAQ page walk — PageStart `0x2a`, **both** PageMessage variants `0x25` *and* `0x26`, PageButton `0x24`, PageEnd `0x28` |
| `intelliflo.cap` | Pentair `0xA5` IntelliFlo status (16-bit BE RPM) |

> Generated fixtures are the right tool for **decoder unit tests** and for **new** wire forms we
> recovered but the app may not handle yet (e.g. the `0x26` long PageMessage, the `0x29`/`0x2a`
> config-dependent page type) — they turn "we found this byte layout" into a runnable assertion.

## B. Live-captured fixtures (high-fidelity, from the sims)

The sims emit *genuine* vendor traffic. To record it with the app's `--record-serial`, the
master sim, the device sim(s) and the app must share one **virtual RS-485 multidrop bus**. On
Windows the reliable way is **com0com** (virtual COM pairs) + **hub4com** (its bundled N-way
broadcast hub):

```
            ┌──────── hub4com (broadcast bus) ────────┐
 Pwrcntr ──►│ CNCB0   CNCB1   CNCB2                    │
 sim     ◄──│   │       │       │                      │
            └───┼───────┼───────┼──────────────────────┘
        CNCA0 ──┘  CNCA1┘  CNCA2┘
          │          │        │
     master COM   device COM   app:  aqualink-automate --serial-port \\.\CNCA2 \
   (Pwrcntr.exe) (Aquarite.exe)        --record-serial session.cap
```

1. Install com0com; create pairs `CNCA0↔CNCB0`, `CNCA1↔CNCB1`, `CNCA2↔CNCB2`.
2. `hub4com --baud=9600 \\.\CNCB0 \\.\CNCB1 \\.\CNCB2` (route bytes among all three).
3. Point `Pwrcntr.exe`'s CommPort at `CNCA0`, the equipment sim's at `CNCA1`.
4. Run the app on `CNCA2` with `--record-serial session.cap` (it only listens + records).
5. Drive the sim UI (set %, change speed, open a page…); stop; copy `session.cap` to
   `test/fixtures/`.

Notes / things to verify when you first wire this up (not yet run end-to-end here):
- The sims also have a NetIO **TCP** bus (`netInit`/`netSendMsg`), so an alternative is to let
  the sims interconnect over TCP and bridge a single endpoint to a COM port (or to the app's
  `--remote-serial-port host:port --rfc2217`). com0com+hub4com is the most general path.
- Baud/parity must match the app's serial settings.
- A `--record-serial` file is already in the exact fixture format — no conversion needed.

This path is what produces **per-model/config** captures: set `Pwrcntr.exe`'s `ControllerType`
(INI / EEPROM.DAT) to an **Only**, a **Combo** and a **Dual** model (see
[pwrcntr-behavior.md](pwrcntr-behavior.md) §1) and capture each — that exercises the
config-dependent branches (spa logic, `0x29`/`0x2a` page type, freeze protection) that a single
hardware unit can't.

---

## Turning a fixture into a behaviour test

Both sources feed the same harness (`test/unit/support/unit_test_mockreplayharness.h` +
`unit_test_loadfixture.h`). The fixture replays through the **real** Jandy decode stack, so
assertions are true end-to-end behaviour (per [docs/RECORD_REPLAY.md](../RECORD_REPLAY.md) §4):

```cpp
Test::MockReplayHarness harness;
auto id  = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(0x50));
auto& sw = harness.AddDevice<Devices::AquariteDevice>(id);

Test::ReplayFixture(harness, "fixtures/alwin32/chlorinator.cap");

BOOST_REQUIRE_EQUAL(harness.DataHub()->Chlorinators().size(), 1u);
BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), 3200.0);
```

## Suggested behaviour-test matrix

| Dimension | Cases to cover | Source |
|-----------|----------------|--------|
| Device decode | chlorinator, ePump, IntelliFlo, heaters, ChemLink, aux, keypad | generated + captured |
| Controller model | RS-x **Only** vs **Combo** vs **Dual** (`ControllerType`) | captured (set per run) |
| Pool/spa | spa on/off, spillover, freeze protection on (pump-lock, aux-on) | captured |
| IAQ pages | EquipmentStatus walk, `0x25` **and** `0x26` lines, `0x2a`/`0x29` page-type, PageEnd page-no | generated + captured |
| iAqualink2 | `UAPI Client.exe` TLV status `0x82`–`0x86` (temp = byte−20, pump SWITCHING) | captured (UAPI ↔ Pwrcntr) |
| Serial-adapter | `Seradptr.exe` ASCII `!KEYWORD`/`?KEYWORD =` (host-facing, separate decoder) | captured |

## Status / what's proven here

- **Generated track: working and validated** — `--selftest` reproduces the committed sample's
  framing exactly; `chlorinator.cap` is byte-equivalent in form to the app's own passing e2e
  fixture. The other generated `.cap`s follow the documented per-device layouts (the IAQ-`0x26`
  and `intelliflo` ones intentionally exercise forms the app may not yet decode — they are the
  spec for that work).
- **Live track: recipe documented, not yet run end-to-end** (needs com0com installed + the sim
  COM/baud wired). Worth doing once to mint real per-model captures and to confirm the generated
  fixtures against vendor bytes.
