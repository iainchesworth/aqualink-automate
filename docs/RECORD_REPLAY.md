# Serial Record / Replay

Aqualink-Automate can **record** the raw RS-485 serial traffic it exchanges with
real hardware to a capture file, and later **replay** that file instead of a real
port. The recorder and replayer share one on-disk format, so a capture records
cleanly and replays as-is — including as a reusable test fixture.

- Recorder: `src/core/developer/recording_serial_port_impl.cpp`
- Replayer: `src/core/developer/mock_serial_port_impl.cpp` (`HandleFileRead` / `DecodeReplayLine`)

---

## 1. Record against real hardware

Recording wraps the *real* serial port (physical or network), so you must point
the app at actual hardware as well as giving it a capture path:

```sh
# Physical RS-485 adapter
aqualink-automate --serial-port /dev/ttyUSB0 --record-serial session.cap

# Networked RS-485 (RFC2217 / raw socket)
aqualink-automate --remote-serial-port 192.168.1.50:6638 --rfc2217 --record-serial session.cap
```

Every byte read from the bus is appended to `session.cap` as it arrives (writes
the app sends are recorded too — see R/W below). The file is flushed per line, so
a capture is usable even if the process is killed.

`--record-serial` conflicts with `--replay-filename` (you cannot record and
replay at once).

## 2. Replay a capture

Replaying needs developer mode and a capture file; no hardware is touched:

```sh
aqualink-automate --dev-mode --replay-filename session.cap
```

The app feeds the capture's **R-direction** bytes into the decode pipeline
exactly as if they had arrived from a real device. `--replay-filename` requires
`--dev-mode`.

### Replay pacing

A real bus delivers bytes at a fixed rate; a capture file does not, so by
default the replayer paces itself to roughly the bus's natural inter-frame
period instead of consuming the whole file as fast as the parser will accept it.
Pacing is applied in the protocol read/parse loop (one serial chunk read per
cycle, then a sleep) — never by blocking the serial read — so framing/sync is
identical to a live port. As a side effect it also keeps the read aligned with
the fixed-size circular buffer; an unpaced replay of a long capture slurps the
whole file in one cycle and overruns the buffer, losing everything but the tail.

| Flag | Default | Meaning |
|------|---------|---------|
| `--replay-frame-period <ms>` | `15` | Wall-clock period between read/parse cycles. `0` = unpaced (as fast as possible — the old behaviour). |
| `--replay-speed <factor>` | `1.0` | Scales the period: `>1` faster, `<1` slower (e.g. `--replay-speed 10` ≈ 1.5 ms/cycle). |

```sh
# Default ≈15 ms/cycle pacing
aqualink-automate --dev-mode --replay-filename session.cap

# 10x faster (still paced; no buffer overflow)
aqualink-automate --dev-mode --replay-filename session.cap --replay-speed 10

# Unpaced — consume the capture as fast as possible
aqualink-automate --dev-mode --replay-filename session.cap --replay-frame-period 0
```

Pacing applies only to the `--replay-filename` path; real serial ports already
self-pace, and the unit-test replay harness runs unpaced for speed.

---

## 3. Capture file format

A capture is a UTF-8 text file:

```
# Serial recording started at: Sun Jun 08 12:00:00 2026
# Format: [timestamp_ms] direction byte|byte|byte|...
# Direction: R=read (from device), W=write (to device)
#
[       142] R 0x10|0x02|0x50|0x14|0x76|0x10|0x03
[       310] W 0x10|0x02|0x50|0x11|0x28|0x9b|0x10|0x03
```

Line kinds:

| Line                              | Meaning                                             | On replay |
|-----------------------------------|-----------------------------------------------------|-----------|
| `# ...`                           | comment / header / metadata                         | ignored   |
| *(blank / whitespace-only)*       | spacing                                             | ignored   |
| `[<ts>] R 0x##\|0x##\|...`         | bytes **read from** the device (the incoming stream) | **replayed** |
| `[<ts>] W 0x##\|0x##\|...`         | bytes the app **wrote to** the device (its own output) | ignored  |
| `0x##\|0x##\|...`                  | legacy bare format (no timestamp/direction)         | **replayed** (back-compat) |

- `<ts>` is milliseconds since recording start (informational; replay does not
  reproduce the recorded timestamps — it paces to a fixed `--replay-frame-period`
  instead; see *Replay pacing* above).
- Each byte token is exactly `0x##` (two hex digits). Pipe-separated.
- **Only `R` lines and legacy bare lines are replayed.** `W` lines are the
  application's *past output*; feeding them back as input would corrupt the
  stream, so they are skipped. Malformed tokens are skipped individually; the
  rest of the line still replays.

---

## 4. Use a capture as a test fixture

Drop a capture under `test/fixtures/` (CMake copies that directory next to the
test executables as a POST_BUILD step, so it resolves from the test working
directory). A reference capture lives at `test/fixtures/sample_session.cap`
(an AquaRite SWG identification + status session that populates a chlorinator in
the DataHub).

Load it with the helper in `test/unit/support/unit_test_loadfixture.h`:

```cpp
#include "support/unit_test_loadfixture.h"
#include "support/unit_test_mockreplayharness.h"

// (a) Just the bytes (R-direction only), parsed by the SAME production reader
//     used by --replay-filename, so it is byte-for-byte what a live replay sees:
std::vector<uint8_t> bytes = Test::LoadFixture("fixtures/sample_session.cap");

// (b) Or replay straight through the full Jandy decode stack and assert state:
Test::MockReplayHarness harness;
auto device_id = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(0x50));
auto& swg = harness.AddDevice<Devices::AquariteDevice>(device_id);

Test::ReplayFixture(harness, "fixtures/sample_session.cap");   // = LoadFixture + harness.Replay

BOOST_REQUIRE_EQUAL(harness.DataHub()->Chlorinators().size(), 1u);
BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), 3200.0);
```

Helper API (`AqualinkAutomate::Test`):

- `std::vector<uint8_t> LoadFixture(const std::string& path)` — returns the
  replayable bytes; throws `std::runtime_error` if the file is missing.
- `std::vector<uint8_t> ReplayFixture(MockReplayHarness&, const std::string& path)`
  — `LoadFixture` then `harness.Replay(...)`; returns the bytes replayed.

Relative paths resolve against the test working directory; absolute paths are
used as-is.

### Recording a fixture from a session you captured

A file produced by `--record-serial` (section 1) is already in this exact format
— copy it straight into `test/fixtures/` and load it with `LoadFixture`. No
conversion step is needed; that round-trip (record → replay-as-fixture) is
covered by `test/unit/protocol/test_record_replay_roundtrip.cpp`.
