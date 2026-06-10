#!/usr/bin/env python3
"""Generate Aqualink-Automate replay fixtures (.cap) from the protocol formats
recovered from the official Jandy Alwin32 simulator suite.

The on-disk format is the one the app's recorder/replayer share (see
docs/RECORD_REPLAY.md): lines `[<ts_ms>] R 0x##|0x##|...`. Only R lines are
replayed; W lines are the app's own past writes and are ignored on replay.

Frame = DLE STX dest cmd data... cksum DLE ETX
  cksum = (0x10 + 0x02 + dest + cmd + sum(data)) & 0xFF   (running 8-bit sum)
  any 0x10 in {dest,cmd,data,cksum} is DLE-stuffed with a following 0x00.

Self-test (`python generate_fixtures.py --selftest`) reproduces the bytes of the
committed test/fixtures/sample_session.cap, proving the framing matches the app's
MessageBuilder.
"""
import argparse, os, sys

DLE, STX, ETX = 0x10, 0x02, 0x03


def frame(dest, cmd, data=()):
    """Return the framed+checksummed+DLE-stuffed wire bytes for one Jandy message."""
    data = list(data)
    cksum = (DLE + STX + dest + cmd + sum(data)) & 0xFF
    out = [DLE, STX]
    for b in [dest, cmd, *data, cksum]:
        out.append(b)
        if b == DLE:            # DLE-stuff: distinguish payload 0x10 from framing
            out.append(0x00)
    out += [DLE, ETX]
    return out


def pentair_frame(dest, src, cmd, data=()):
    """Pentair IntelliFlo 0xA5 frame: FF 00 FF A5 00 dest src cmd len data... ck(16-bit BE)."""
    data = list(data)
    body = [0x00, dest, src, cmd, len(data), *data]
    ck = (0xA5 + sum(body)) & 0xFFFF
    return [0xFF, 0x00, 0xFF, 0xA5, *body, (ck >> 8) & 0xFF, ck & 0xFF]


class Cap:
    """Accumulates R/W lines and renders a .cap file."""
    def __init__(self, title, lines_desc):
        self.title = title
        self.desc = lines_desc
        self.events = []          # (ts, dir, [bytes])
        self.ts = 0

    def _add(self, direction, by, gap=120):
        self.ts += gap
        self.events.append((self.ts, direction, by))

    def rx(self, by, gap=150):    # bytes arriving FROM the bus (replayed)
        self._add('R', by, gap); return self

    def tx(self, by, gap=120):    # the app's own output (ignored on replay)
        self._add('W', by, gap); return self

    # convenience: a master->device command (app reads it off the bus)
    def master(self, dest, cmd, data=(), gap=150):
        return self.rx(frame(dest, cmd, data), gap)

    # a device->master reply (dest 0x00)
    def device(self, cmd, data=(), gap=150):
        return self.rx(frame(0x00, cmd, data), gap)

    def render(self):
        out = ["# Serial recording started at: (generated fixture)",
               "# Format: [timestamp_ms] direction byte|byte|byte|...",
               "# Direction: R=read (from device), W=write (to device)",
               "#",
               f"# {self.title}"]
        for d in self.desc:
            out.append(f"#   {d}")
        out.append("#")
        for ts, dr, by in self.events:
            toks = "|".join(f"0x{b:02x}" for b in by)
            out.append(f"[{ts:10}] {dr} {toks}")
        out.append("# Recording ended")
        return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# Scenario builders (driven by the decoded per-device command/payload tables)
# ---------------------------------------------------------------------------

def chlorinator():
    """AquaRite SWG (0x50): GetId, set 40%, report 3200 PPM + status. Mirrors the
    repo's sample_session.cap so it populates a chlorinator in the DataHub."""
    c = Cap("AquaRite SWG (0x50) identify + status -> chlorinator in DataHub",
            ["AQUARITE_GetId 0x14, AQUARITE_Percent 0x11 = 40%,",
             "AQUARITE_PPM 0x16 = 3200 ppm (byte/100=0x20), status On."])
    c.master(0x50, 0x14)                 # GetId
    c.master(0x50, 0x11, [0x28])         # set 40%
    c.device(0x16, [0x20, 0x00])         # PPM=3200, status On
    return c


def epump():
    """Jandy ePump (0x78): master 'D' set-speed (RPM x4 LE) + 'A' poll, pump status."""
    rpm = 2400
    v = rpm * 4
    c = Cap("Jandy ePump (0x78) set 2400 RPM then poll",
            ["'D'(0x44) set speed = RPM*4 little-endian; 'A'(0x41) poll;",
             "pump status echoes mode + value (see docs/alwin32/epump.md)."])
    c.master(0x78, 0x44, [v & 0xFF, (v >> 8) & 0xFF])   # set speed
    c.master(0x78, 0x41)                                 # poll
    c.device(0x41, [0x0B, v & 0xFF, (v >> 8) & 0xFF, 0, 0])  # status-ish
    return c


def heater():
    """LX heater (0x38): master 0x0C enable+setpoints, heater 0x0D status (Heating)."""
    c = Cap("LX heater (0x38) enable with setpoints; reports Heating",
            ["0x0C = [flags][PoolSP][SpaSP][MeasTemp]; flags bit3=Heat Cmd, bit4=temp present;",
             "0x0D reply = [status][temp][switchBits]; status bit3(0x08)=Heating."])
    c.master(0x38, 0x0C, [0x09, 80, 102, 78])   # heat-cmd+temp-present, pool SP 80, spa 102, meas 78
    c.device(0x0D, [0x08, 78, 0x00])            # Heating, temp 78, no faults
    return c


def iaq_page():
    """IAQ page sequence to AqualinkTouch: PageStart(0x2a) -> PageMessage 0x25 + 0x26
    -> PageButton 0x24 -> PageEnd 0x28. Exercises BOTH page-message variants."""
    c = Cap("IAQ EquipmentStatus page walk (master -> AqualinkTouch 0x33)",
            ["0x23 PageStart payload 0x2a=EquipmentStatus; 0x25 short line [line][text];",
             "0x26 long line [line][attr][text]; 0x24 PageButton [idx][state][u16][l1][l2];",
             "0x28 PageEnd [b1][b2][pageNo+'0'][b4][b5]. See docs/alwin32/pwrcntr-behavior.md."])
    def s(text): return [ord(ch) for ch in text] + [0]
    c.master(0x33, 0x23, [0x2a])                       # PageStart, EquipmentStatus
    c.master(0x33, 0x25, [0x00] + s("EQUIPMENT"))      # short line 0: title
    c.master(0x33, 0x26, [0x01, 0x00] + s("Pool Pump  ON"))   # long line 1
    c.master(0x33, 0x26, [0x02, 0x00] + s("Spa        OFF"))  # long line 2
    c.master(0x33, 0x24, [0x01, 0x01, 0x00, 0x00] + s("Pool") + s("ON"))  # button
    c.master(0x33, 0x28, [0x00, 0x00, ord('1'), 0x00, 0x00])  # PageEnd, page "1"
    return c


def intelliflo():
    """Pentair IntelliFlo (0x60): request status (0x07) -> 0xA5 status frame."""
    c = Cap("Pentair IntelliFlo (0x60) request-status -> 0xA5 status",
            ["Pentair 0xFF 00 FF A5 framing, 16-bit BE checksum; RPM 16-bit big-endian.",
             "See docs/alwin32/iflo.md. (Carried over the bus; framing differs from Jandy.)"])
    # status frame: running 2400 RPM, commanded 2400, mode running
    rpm = 2400
    data = [0]*15
    data[8] = (rpm >> 8) & 0xFF; data[9] = rpm & 0xFF      # running RPM (BE) at payload[12-13]->offset
    data[10] = (rpm >> 8) & 0xFF; data[11] = rpm & 0xFF
    data[12] = 0x4B                                         # mode running
    c.rx(pentair_frame(0x10, 0x60, 0x07, data))
    return c


SCENARIOS = {
    "chlorinator": chlorinator,
    "epump": epump,
    "heater": heater,
    "iaq_page": iaq_page,
    "intelliflo": intelliflo,
}


def selftest():
    """Reproduce the R-frames of the committed sample_session.cap, proving the
    framing/checksum matches the app's MessageBuilder."""
    expect = {
        "GetId":   ([0x10, 0x02, 0x50, 0x14, 0x76, 0x10, 0x03], frame(0x50, 0x14)),
        "Percent": ([0x10, 0x02, 0x50, 0x11, 0x28, 0x9b, 0x10, 0x03], frame(0x50, 0x11, [0x28])),
        "PPM":     ([0x10, 0x02, 0x00, 0x16, 0x20, 0x00, 0x48, 0x10, 0x03], frame(0x00, 0x16, [0x20, 0x00])),
    }
    ok = True
    for name, (want, got) in expect.items():
        status = "OK" if want == got else "FAIL"
        if want != got:
            ok = False
        print(f"  [{status}] {name}: {' '.join(f'{b:02x}' for b in got)}")
    print("SELFTEST", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--out", default=os.path.dirname(os.path.abspath(__file__)))
    ap.add_argument("--only", nargs="*", help="subset of scenarios to write")
    args = ap.parse_args()
    if args.selftest:
        sys.exit(selftest())
    names = args.only or list(SCENARIOS)
    for name in names:
        cap = SCENARIOS[name]()
        path = os.path.join(args.out, f"{name}.cap")
        with open(path, "w", encoding="utf-8") as f:
            f.write(cap.render())
        print(f"wrote {path}")
