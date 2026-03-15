#!/usr/bin/env python3
"""
test_aegis.py — UART test harness for Aegis-Chip CC-CBF vector calculation.

Usage:
    python3 test_aegis.py                       # auto-detect port, run all tests
    python3 test_aegis.py /dev/ttyUSB1          # specify port
    python3 test_aegis.py --loopback            # wire RX←TX, verify UART only
    python3 test_aegis.py --interactive         # type your own x_t values
    python3 test_aegis.py --single "0,0,...,0"  # run one custom vector

Protocol (matches uart_rx.v / uart_tx.v):
    TX to FPGA  : 32 bytes = 16 × INT16, little-endian (LSB byte first per word)
    RX from FPGA: 32 bytes = 16 × INT16, little-endian
    Baud rate   : 115200, 8N1
"""

import sys
import struct
import time
import glob
import argparse
import serial

# ---------------------------------------------------------------------------
# Chip constants  — must match hls/aegis_shield/aegis_shield.h
# ---------------------------------------------------------------------------
DIM     = 16
W       = [64, 128, 96, 112, 80, 72, 104, 88, 120, 60, 92, 116, 76, 84, 100, 68]
B_BASE  = 5000
ALPHA   = 6000
ANXIETY = 0     # default: no buttons pressed. Override with --anxiety or --buttons

# Button → anxiety level lookup (KEY1=MSB, KEY4=LSB, each step = 64)
# e.g. buttons "1000" (KEY1 only) = 8 * 64 = 512
BTN_LEVELS = [i * 64 for i in range(16)]   # [0, 64, 128, ..., 960]

def buttons_to_anxiety(key1, key2, key3, key4):
    """Convert 4 boolean button states to anxiety_level."""
    idx = (int(bool(key1)) << 3 | int(bool(key2)) << 2 |
           int(bool(key3)) << 1 | int(bool(key4)))
    return idx * 64

def anxiety_to_buttons(level):
    """Return a string showing which buttons to press for a given anxiety level."""
    idx = level // 64
    if idx < 0 or idx > 15 or level % 64 != 0:
        return f"(not a valid button level — must be multiple of 64, 0–960)"
    bits = [(idx >> b) & 1 for b in [3, 2, 1, 0]]
    names = [f"KEY{i+1}" for i, b in enumerate(bits) if b]
    return "none" if not names else " + ".join(names)

# ANSI colours
GRN = "\033[92m"; RED = "\033[91m"; YLW = "\033[93m"; RST = "\033[0m"
PASS = f"{GRN}PASS{RST}"; FAIL = f"{RED}FAIL{RST}"; WARN = f"{YLW}WARN{RST}"

# ---------------------------------------------------------------------------
# Golden model (pure Python — matches fixed-point HLS arithmetic exactly)
# ---------------------------------------------------------------------------
def reference_cbf(x_t, anxiety=ANXIETY):
    """Return (u_t, h) for the given x_t using the same fixed-point logic as the chip."""
    B = B_BASE - ((ALPHA * anxiety) >> 10)
    h = B + sum(W[i] * x_t[i] for i in range(DIM))
    if h >= 0:
        return [0] * DIM, h
    u = [max(-32768, min(32767, ((-h) * w) >> 10)) for w in W]
    return u, h

# ---------------------------------------------------------------------------
# Predefined test vectors
# ---------------------------------------------------------------------------
TEST_CASES = [
    # ── Safe-zone tests ──────────────────────────────────────────────────────
    dict(name="Safe: zero vector  (h = +2000)",
         x_t=[0] * DIM),

    dict(name="Safe: all +1       (h = +3460)",
         x_t=[1] * DIM),

    dict(name="Safe: near barrier (h ≈ +16)",
         x_t=[-31] + [0] * 15,
         note="h = 2000 + 64×(−31) = +16; still safe"),

    dict(name="Safe: max INT16    (h >> 0, no overflow)",
         x_t=[32767] * DIM,
         note="All large positives; u_t must be all-zero"),

    # ── Violation tests ──────────────────────────────────────────────────────
    dict(name="Violation: all −2  (h = −920)",
         x_t=[-2] * DIM,
         note="u_t[0]=57, u_t[1]=115 — small proportional correction"),

    dict(name="Violation: all −100 (h = −144000)",
         x_t=[-100] * DIM,
         note="u_t[0]=9000, u_t[1]=18000 — large steering"),

    dict(name="Violation: asymmetric x[0]=−500 only (h = −30000)",
         x_t=[-500] + [0] * 15,
         note="Only W[0]-weighted component non-zero"),

    # ── Saturation / edge tests ──────────────────────────────────────────────
    dict(name="Saturation: all −1000 (some u_t saturate at 32767)",
         x_t=[-1000] * DIM),

    dict(name="Alternating +10/−20 (partial violation)",
         x_t=[10, -20] * 8),

    dict(name="Impulse: x[7]=−200, rest zero",
         x_t=[0]*7 + [-200] + [0]*8,
         note="Only W[7]=88 contributes; h = 2000 + 88×(−200) = −15600"),
]

# ---------------------------------------------------------------------------
# UART helpers
# ---------------------------------------------------------------------------
def pack_vector(x_t):
    """16 × INT16 → 32 bytes, little-endian."""
    return struct.pack('<' + 'h' * DIM, *x_t)

def unpack_vector(data):
    """32 bytes → list of 16 INT16."""
    return list(struct.unpack('<' + 'h' * DIM, data))

def open_port(port_name, timeout=0.1):
    return serial.Serial(
        port=port_name, baudrate=115200,
        bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE, timeout=timeout)

def transact(port, x_t, timeout_s=0.5):
    """Send x_t, read back u_t. Returns (vector_or_None, elapsed_ms)."""
    port.reset_input_buffer()
    t0 = time.monotonic()
    port.write(pack_vector(x_t))
    port.flush()

    rx, deadline = b'', time.monotonic() + timeout_s
    while len(rx) < 32 and time.monotonic() < deadline:
        rx += port.read(32 - len(rx))

    elapsed_ms = (time.monotonic() - t0) * 1000.0
    return (unpack_vector(rx) if len(rx) == 32 else None), elapsed_ms

def fmt(v, n=6):
    s = ", ".join(str(x) for x in v[:n])
    if len(v) > n:
        s += f", … ({len(v)-n} more)"
    return "[" + s + "]"

# ---------------------------------------------------------------------------
# Loopback test (wire UART_RXD ← UART_TXD on the board header)
# ---------------------------------------------------------------------------
def run_loopback(port_name):
    """Send 32 bytes and verify we receive the exact same bytes back."""
    print(f"\n{YLW}Loopback Test{RST} — wire UART_TXD → UART_RXD on the expansion header")
    print("This ONLY tests the UART wiring; the FPGA logic is bypassed.\n")
    port = open_port(port_name, timeout=0.2)
    time.sleep(0.05)

    x_t = list(range(-8, 8))   # distinctive pattern
    payload = pack_vector(x_t)
    port.reset_input_buffer()
    port.write(payload)
    port.flush()

    rx = b''
    deadline = time.monotonic() + 1.0
    while len(rx) < 32 and time.monotonic() < deadline:
        rx += port.read(32 - len(rx))

    port.close()
    if rx == payload:
        print(f"[{PASS}] Received exactly what was sent — UART RX/TX wiring OK.")
    elif len(rx) == 0:
        print(f"[{FAIL}] No bytes received. Check: cable, port name, board power.")
    else:
        print(f"[{FAIL}] Got {len(rx)}/32 bytes, mismatch.")
        print(f"  sent: {payload.hex()}")
        print(f"  got : {rx.hex()}")

# ---------------------------------------------------------------------------
# Main test suite
# ---------------------------------------------------------------------------
def run_tests(port_name, anxiety=None):
    anx = anxiety if anxiety is not None else ANXIETY
    B = B_BASE - ((ALPHA * anx) >> 10)
    print(f"\n{'='*68}")
    print(f"  Aegis-Chip CC-CBF Test Harness")
    print(f"  Port: {port_name}   Baud: 115200")
    print(f"  anxiety_level = {anx}  ({anxiety_to_buttons(anx)})")
    print(f"  B = {B_BASE} − (({ALPHA}×{anx})>>10) = {B}")
    print(f"  W = {W[:8]}")
    print(f"      {W[8:]}")
    print(f"{'='*68}\n")

    port = open_port(port_name)
    time.sleep(0.05)

    passed = failed = timeouts = 0
    rows = []

    for tc in TEST_CASES:
        x_t      = tc["x_t"]
        expected, h = reference_cbf(x_t, anxiety=anx)
        note     = tc.get("note", "")

        received, ms = transact(port, x_t)

        if received is None:
            status  = FAIL
            detail  = f"{RED}TIMEOUT — no response in 500 ms{RST}"
            timeouts += 1
        elif received == expected:
            status  = PASS
            detail  = f"u_t = {fmt(received)}"
            passed += 1
        else:
            status  = FAIL
            got_s   = fmt(received)
            exp_s   = fmt(expected)
            detail  = f"got {RED}{got_s}{RST}\n{'':12}exp {GRN}{exp_s}{RST}"
            failed += 1

        print(f"[{status}] {tc['name']}")
        if note:
            print(f"           {YLW}{note}{RST}")
        print(f"           x_t = {fmt(x_t)}   h = {h}")
        print(f"           {detail}   ({ms:.1f} ms)\n")
        rows.append((tc["name"], status, ms))

    port.close()

    print("="*68)
    print(f"Results: {GRN}{passed} passed{RST}, {RED}{failed} failed{RST}, "
          f"{YLW}{timeouts} timeouts{RST}  / {len(TEST_CASES)} total")

    if failed == 0 and timeouts == 0:
        print(f"\n{GRN}✓ All tests passed — CC-CBF computation verified on hardware.{RST}")
        print("  The aegis_shield IP is computing steering vectors correctly.")
        print("  You can now connect your LLM host and set anxiety_level dynamically.")
    else:
        _print_debug_hints(timeouts, failed)
    print()

def _print_debug_hints(timeouts, failed):
    print(f"\n{RED}✗ Tests failed. Diagnostics:{RST}")
    if timeouts > 0:
        print("  Timeouts — check:")
        print("    • Bitstream programmed?  ls -lh impl/aegis_chip.bit")
        print("    • Correct serial port?   ls /dev/ttyUSB*")
        print("    • Board powered?  (PWR LED lit)")
        print("    • DONE LED lit after programming?")
        print("    • Baud rate: CLKS_PER_BIT = 200_000_000 / 115200 = 1736")
    if failed > 0:
        print("  Wrong values — check:")
        print("    • anxiety_level in rtl/aegis_top.v — must be 16'd512")
        print("    • W[] array in hls/aegis_shield/aegis_shield.h matches ANXIETY here")
        print("    • Re-run HLS+synthesis if you edited any constants")

# ---------------------------------------------------------------------------
# Interactive mode
# ---------------------------------------------------------------------------
def run_interactive(port_name, anxiety=None):
    anx = anxiety if anxiety is not None else ANXIETY
    port = open_port(port_name)
    B = B_BASE - ((ALPHA * anx) >> 10)
    print(f"\n{YLW}Interactive mode{RST}  (type 'quit' to exit)")
    print(f"Enter 16 comma-separated INT16 values for x_t.")
    print(f"anxiety_level={anx}  ({anxiety_to_buttons(anx)}),  B={B}\n")

    while True:
        try:
            raw = input("x_t> ").strip()
        except (EOFError, KeyboardInterrupt):
            break
        if raw.lower() in ("q", "quit", "exit"):
            break
        try:
            vals = [int(v.strip()) for v in raw.split(",")]
            if len(vals) != DIM:
                print(f"  Need {DIM} values, got {len(vals)}")
                continue
            if not all(-32768 <= v <= 32767 for v in vals):
                print("  Values must be in range [-32768, 32767]")
                continue
        except ValueError:
            print("  Invalid input — enter integers separated by commas")
            continue

        expected, h = reference_cbf(vals, anxiety=anx)
        received, ms = transact(port, vals)

        print(f"  h          = {h}")
        print(f"  expected   = {expected}")
        if received is None:
            print(f"  received   = {RED}TIMEOUT{RST}")
        elif received == expected:
            print(f"  received   = {GRN}{received}{RST}  ({ms:.1f} ms)  {PASS}")
        else:
            print(f"  received   = {RED}{received}{RST}  ({ms:.1f} ms)  {FAIL}")
        print()

    port.close()

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def _run_scan(port_name):
    """
    Scan mode: sweep all 16 anxiety levels with a fixed mild-violation vector.
    Prompts the user to set each button combination, then sends the vector
    and verifies the result changes correctly as anxiety_level grows.
    """
    X_T = [-30] * DIM    # mild violation at anxiety=0 (h=2000-43800=-41800)
    print(f"\n{YLW}Button Scan Mode{RST} — sweeps all 16 anxiety levels")
    print(f"Fixed test vector x_t = {X_T[:4]}…  (all -30)")
    print("For each level the script tells you which buttons to press,")
    print("then waits for you to press ENTER before sending the vector.\n")

    port   = open_port(port_name)
    passed = 0

    for level in BTN_LEVELS:
        B   = B_BASE - ((ALPHA * level) >> 10)
        exp, h = reference_cbf(X_T, anxiety=level)
        btns = anxiety_to_buttons(level)

        print(f"  [{level:>4d}]  Press: {YLW}{btns:<30s}{RST} ", end="", flush=True)
        input("then press ENTER → ")

        got, ms = transact(port, X_T)
        ok = got == exp

        if ok:
            passed += 1
            print(f"         {PASS}  h={h:>9}  u_t[0..3]={got[:4]}  ({ms:.1f} ms)")
        else:
            timeout = got is None
            if timeout:
                print(f"         {FAIL}  {RED}TIMEOUT{RST}")
            else:
                print(f"         {FAIL}  got={got[:4]}, exp={exp[:4]}")

    port.close()
    print(f"\n{passed}/{len(BTN_LEVELS)} levels passed.")


def auto_detect_port():
    candidates = sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))
    if not candidates:
        print(f"{RED}No USB serial ports found.{RST} Is the USB cable connected?")
        sys.exit(1)
    if len(candidates) > 1:
        print(f"{YLW}Multiple ports found:{RST} {candidates}")
        print(f"Using {candidates[0]}. Pass a port name to override.")
    return candidates[0]

def main():
    parser = argparse.ArgumentParser(
        description="Aegis-Chip UART test harness for CC-CBF vector calculation")
    parser.add_argument("port", nargs="?",
                        help="Serial port (default: auto-detect /dev/ttyUSB*)")
    parser.add_argument("--loopback", action="store_true",
                        help="UART loopback test only (wire RX←TX)")
    parser.add_argument("--interactive", action="store_true",
                        help="Enter custom x_t vectors interactively")
    parser.add_argument("--single",
                        help='Run one vector, e.g. --single "-100,-100,...,-100"')

    # Button / anxiety controls
    anx_grp = parser.add_mutually_exclusive_group()
    anx_grp.add_argument("--anxiety", type=int, default=None,
                         metavar="LEVEL",
                         help="Anxiety level (0–960, multiple of 64). "
                              "Set the board buttons to match before running.")
    anx_grp.add_argument("--buttons", metavar="KEY1,KEY2,KEY3,KEY4",
                         help="Button state as 4 booleans, e.g. --buttons 1,0,0,0 "
                              "(KEY1 pressed only → anxiety=512)")
    parser.add_argument("--scan", action="store_true",
                        help="Scan all 16 button levels with a fixed test vector")
    args = parser.parse_args()

    port = args.port or auto_detect_port()

    # Resolve anxiety level from --anxiety or --buttons
    anx = None
    if args.anxiety is not None:
        if args.anxiety not in BTN_LEVELS:
            print(f"--anxiety must be a multiple of 64 in [0..960]. Got {args.anxiety}")
            sys.exit(1)
        anx = args.anxiety
    elif args.buttons:
        try:
            b = [int(x.strip()) for x in args.buttons.split(",")]
            if len(b) != 4:
                raise ValueError
            anx = buttons_to_anxiety(*b)
        except (ValueError, TypeError):
            print("--buttons needs 4 comma-separated 0/1 values, e.g. 1,0,0,0")
            sys.exit(1)

    try:
        if args.scan:
            _run_scan(port)
            sys.exit(0)
        elif args.loopback:
            run_loopback(port)
        elif args.interactive:
            run_interactive(port, anxiety=anx)
        elif args.single:
            vals = [int(v.strip()) for v in args.single.split(",")]
            if len(vals) != DIM:
                print(f"Need {DIM} values, got {len(vals)}")
                sys.exit(1)
            p = open_port(port)
            exp, h = reference_cbf(vals, anxiety=anx or ANXIETY)
            got, ms = transact(p, vals)
            p.close()
            used_anx = anx if anx is not None else ANXIETY
            print(f"anxiety  = {used_anx}  ({anxiety_to_buttons(used_anx)})")
            print(f"x_t      = {vals}")
            print(f"h        = {h}")
            print(f"expected = {exp}")
            print(f"received = {got}  ({ms:.1f} ms)")
            ok = got == exp
            print(f"result   = {PASS if ok else FAIL}")
            sys.exit(0 if ok else 1)
        else:
            run_tests(port, anxiety=anx)

    except serial.SerialException as e:
        print(f"{RED}Cannot open {port}: {e}{RST}")
        print("Available ports:", sorted(
            glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")))
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nInterrupted.")

if __name__ == "__main__":
    main()
