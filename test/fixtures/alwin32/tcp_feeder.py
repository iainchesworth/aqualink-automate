#!/usr/bin/env python3
"""Raw-TCP serial feeder for end-to-end, no-install behaviour testing.

Streams the R-direction bytes of a .cap fixture to a connecting aqualink-automate
instance over a plain TCP socket, and logs the bytes the app writes back (its
commands). The app connects as a TCP serial client:

    python tcp_feeder.py chlorinator.cap --port 9999
    aqualink-automate --remote-serial-port 127.0.0.1:9999 --rawtcp \
        --record-serial roundtrip.cap --loglevel-messages info

This drives the *live* app (full decode stack, DataHub, web/MQTT) with specific
device/behaviour traffic without any virtual-serial driver. The simulators
themselves share an in-memory bus inside NetIO.dll (a Shared PE section) and use
a real COM port only for real hardware, so they cannot be tapped no-install --
see docs/alwin32/sim-connection.md. The fixtures fed here come from
generate_fixtures.py (the decoded vendor formats) or from a com0com live capture.
"""
import argparse
import re
import socket
import threading
import time

R_LINE = re.compile(r"\[\s*\d+\s*\]\s+R\s+(.*)")


def load_cap_rbytes(path):
    """Return the concatenated R-direction (and legacy bare) bytes of a .cap file."""
    out = bytearray()
    with open(path, encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            m = R_LINE.match(s)
            if m:
                toks = m.group(1)
            elif s and not s.startswith("#") and s.startswith("0x"):
                toks = s  # legacy bare "0x..|0x.." line
            else:
                continue
            for t in toks.split("|"):
                t = t.strip()
                if t.startswith("0x"):
                    out.append(int(t, 16) & 0xFF)
    return bytes(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("cap", help=".cap fixture to stream (R lines)")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=9999)
    ap.add_argument("--chunk", type=int, default=16, help="bytes per TCP write")
    ap.add_argument("--interval", type=float, default=0.05, help="seconds between chunks")
    ap.add_argument("--loop", action="store_true", help="repeat the capture continuously")
    ap.add_argument("--write-log", help="append the app's writes to this file as W lines")
    args = ap.parse_args()

    data = load_cap_rbytes(args.cap)
    print(f"[feeder] loaded {len(data)} R-bytes from {args.cap}", flush=True)

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((args.host, args.port))
    srv.listen(1)
    print(f"[feeder] listening on {args.host}:{args.port} -- run the app with "
          f"--remote-serial-port {args.host}:{args.port} --rawtcp", flush=True)

    conn, addr = srv.accept()
    print(f"[feeder] app connected from {addr}", flush=True)
    stop = threading.Event()

    def reader():
        wl = open(args.write_log, "a", encoding="utf-8") if args.write_log else None
        while not stop.is_set():
            try:
                b = conn.recv(4096)
            except OSError:
                break
            if not b:
                break
            print(f"[feeder] app wrote {len(b)} bytes: {' '.join(f'{x:02x}' for x in b)}", flush=True)
            if wl:
                wl.write("W " + "|".join(f"0x{x:02x}" for x in b) + "\n")
                wl.flush()

    threading.Thread(target=reader, daemon=True).start()

    try:
        while True:
            for i in range(0, len(data), args.chunk):
                conn.sendall(data[i:i + args.chunk])
                time.sleep(args.interval)
            print("[feeder] sent full capture", flush=True)
            if not args.loop:
                break
            time.sleep(1.0)
        print("[feeder] holding connection open (Ctrl-C to exit)", flush=True)
        while not stop.is_set():
            time.sleep(1)
    except (KeyboardInterrupt, BrokenPipeError, ConnectionResetError):
        pass
    finally:
        stop.set()
        conn.close()
        srv.close()


if __name__ == "__main__":
    main()
