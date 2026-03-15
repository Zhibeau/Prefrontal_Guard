#!/usr/bin/env python3
"""
test_aegis_usb.py — USB bulk-transfer test harness for Aegis-Chip (CY68013A FX2LP).

Usage:
    python3 test_aegis_usb.py                   # run all test cases
    python3 test_aegis_usb.py --scan-devices    # list detected USB devices
    python3 test_aegis_usb.py --single "-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100"
    python3 test_aegis_usb.py --interactive     # enter custom vectors
    python3 test_aegis_usb.py --single "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0" --prime-frames 2
    python3 test_aegis_usb.py --vid 0x04B4 --pid 0x00F1   # override VID/PID

Protocol (matches aegis_usb_bridge.v):
    TX to FPGA  : 32 bytes = 16 × INT16, little-endian → USB bulk write to EP2
    RX from FPGA: 32 bytes = 16 × INT16, little-endian ← USB bulk read from EP6
    USB         : CY68013A FX2LP, slave-FIFO mode

FX2LP endpoint mapping:
    EP2 OUT (0x02) : host → FPGA  (FPGA reads via usb_flaga / SLRD)
    EP6 IN  (0x86) : FPGA → host  (FPGA writes via usb_flagc / SLWR)
"""

import sys
import struct
import time
import argparse

try:
    import usb.core
    import usb.util
except ImportError:
    print("pyusb not installed.  Run:  pip install pyusb")
    sys.exit(1)

# ---------------------------------------------------------------------------
# CY68013A VID/PID candidates
# The FX2LP defaults to 0x04B4/0x8613 when no EEPROM is present.
# ALINX vendor firmware typically uses 0x04B4/0x00F1.
# ---------------------------------------------------------------------------
USB_CANDIDATES = [
    (0x04B4, 0x00F1),   # ALINX AX7102 CY68013A vendor firmware
    (0x04B4, 0x1004),   # another common ALINX firmware PID
    (0x04B4, 0x8613),   # CY68013A default (no EEPROM / blank)
    (0x04B4, 0x0100),   # FX2LP development kit default
]

EP2_OUT = 0x02   # bulk OUT  → FPGA (host writes here)
EP6_IN  = 0x86   # bulk IN   ← FPGA (host reads here); 0x86 = 0x80 | 6

FRAME_WORDS  = 16
FRAME_BYTES  = FRAME_WORDS * 2   # 32 bytes — payload

# The FPGA pads EP6 writes to 512 bytes (256 × 16-bit words) so the FX2LP
# auto-commits a full USB packet (no PKTEND pin wired on this board).
USB_PACKET_BYTES = 512

# ---------------------------------------------------------------------------
# CC-CBF golden model (matches aegis_shield.v arithmetic exactly)
# ---------------------------------------------------------------------------
W      = [64, 128, 96, 112, 80, 72, 104, 88, 120, 60, 92, 116, 76, 84, 100, 68]
B_BASE = 5000
ALPHA  = 6000

def reference_cbf(x_t, anxiety=0):
    B = B_BASE - ((ALPHA * anxiety) >> 10)
    h = B + sum(W[i] * x_t[i] for i in range(FRAME_WORDS))
    if h >= 0:
        return [0] * FRAME_WORDS, h
    u = [max(-32768, min(32767, ((-h) * w) >> 10)) for w in W]
    return u, h

BTN_LEVELS = [i * 64 for i in range(16)]

def anxiety_to_buttons(level):
    idx = level // 64
    if idx < 0 or idx > 15 or level % 64 != 0:
        return "(invalid)"
    bits = [(idx >> b) & 1 for b in [3, 2, 1, 0]]
    names = [f"KEY{i+1}" for i, b in enumerate(bits) if b]
    return "none" if not names else " + ".join(names)

# ANSI
GRN = "\033[92m"; RED = "\033[91m"; YLW = "\033[93m"; RST = "\033[0m"
PASS = f"{GRN}PASS{RST}"; FAIL = f"{RED}FAIL{RST}"

# ---------------------------------------------------------------------------
# Predefined test cases (same as test_aegis.py)
# ---------------------------------------------------------------------------
TEST_CASES = [
    dict(name="Safe: zero vector  (h = +5000)",   x_t=[0] * 16),
    dict(name="Safe: all +1       (h = +6460)",   x_t=[1] * 16),
    dict(name="Safe: near barrier (h ≈ +16)",
         x_t=[-78] + [0] * 15,
         note="h = 5000 + 64×(−78) = +8; still safe"),
    dict(name="Safe: max INT16    (no correction)", x_t=[32767] * 16),
    dict(name="Violation: all −2  (h negative)",  x_t=[-2] * 16),
    dict(name="Violation: all −100",              x_t=[-100] * 16),
    dict(name="Violation: x[0]=−500 only",
         x_t=[-500] + [0] * 15),
    dict(name="Saturation: all −1000",            x_t=[-1000] * 16),
    dict(name="Alternating +10/−20",              x_t=[10, -20] * 8),
    dict(name="Impulse: x[7]=−200, rest zero",
         x_t=[0]*7 + [-200] + [0]*8),
]

# ---------------------------------------------------------------------------
# USB device helpers
# ---------------------------------------------------------------------------
def find_device(vid=None, pid=None):
    """Return (dev, vid, pid) for the first matching CY68013A, or None."""
    candidates = [(vid, pid)] if (vid and pid) else USB_CANDIDATES
    for v, p in candidates:
        dev = usb.core.find(idVendor=v, idProduct=p)
        if dev is not None:
            return dev, v, p
    return None, None, None


def open_device(vid=None, pid=None):
    dev, v, p = find_device(vid, pid)
    if dev is None:
        print(f"{RED}CY68013A FX2LP not found.{RST}")
        print("Tried VID/PID combinations:")
        for vv, pp in ([(vid, pid)] if (vid and pid) else USB_CANDIDATES):
            print(f"  0x{vv:04X} / 0x{pp:04X}")
        print("\nDiagnostics:")
        print("  1. Is the USB cable connected to the board's CY68013A port?")
        print("  2. Run  lsusb  to see what VID/PID the device presents.")
        print("  3. Pass  --vid 0x04B4 --pid <pid>  to override.")
        print("  4. Does the FX2LP need firmware loaded? Check ALINX docs.")
        sys.exit(1)

    # Detach kernel driver if necessary
    if dev.is_kernel_driver_active(0):
        try:
            dev.detach_kernel_driver(0)
        except usb.core.USBError as e:
            print(f"{YLW}Warning: could not detach kernel driver: {e}{RST}")

    try:
        dev.set_configuration()
    except usb.core.USBError as e:
        print(f"{RED}set_configuration() failed: {e}{RST}")
        print("Try running with sudo, or add a udev rule:")
        print('  SUBSYSTEM=="usb", ATTR{idVendor}=="04b4", MODE="0666"')
        sys.exit(1)

    print(f"Found CY68013A FX2LP — VID=0x{v:04X} PID=0x{p:04X}")
    return dev


def scan_devices():
    """Print all connected USB devices."""
    import usb.core
    print("All connected USB devices:")
    for dev in usb.core.find(find_all=True):
        try:
            mfg = usb.util.get_string(dev, dev.iManufacturer) if dev.iManufacturer else ""
            prod = usb.util.get_string(dev, dev.iProduct) if dev.iProduct else ""
        except Exception:
            mfg = prod = ""
        print(f"  VID=0x{dev.idVendor:04X}  PID=0x{dev.idProduct:04X}  {mfg} {prod}".rstrip())


# ---------------------------------------------------------------------------
# Frame transport
# ---------------------------------------------------------------------------
def pack_vector(x_t):
    return struct.pack('<' + 'h' * FRAME_WORDS, *x_t)

def unpack_vector(data):
    return list(struct.unpack('<' + 'h' * FRAME_WORDS, data))

def transact(dev, x_t, timeout_ms=2000):
    """
    Send x_t as 32 bytes to EP2 OUT, receive 32 bytes from EP6 IN.
    Returns (received_vector_or_None, elapsed_ms).
    """
    payload = pack_vector(x_t)
    t0 = time.monotonic()

    try:
        written = dev.write(EP2_OUT, payload, timeout=timeout_ms)
        if written != FRAME_BYTES:
            return None, (time.monotonic() - t0) * 1000
    except usb.core.USBTimeoutError:
        return None, (time.monotonic() - t0) * 1000
    except usb.core.USBError as e:
        print(f"  {RED}Write error: {e}{RST}")
        return None, (time.monotonic() - t0) * 1000

    try:
        # Read full 512-byte USB packet (FPGA pads to this size for auto-commit)
        rx = bytes(dev.read(EP6_IN, USB_PACKET_BYTES, timeout=timeout_ms))
    except usb.core.USBTimeoutError:
        return None, (time.monotonic() - t0) * 1000
    except usb.core.USBError as e:
        print(f"  {RED}Read error: {e}{RST}")
        return None, (time.monotonic() - t0) * 1000

    elapsed_ms = (time.monotonic() - t0) * 1000
    if len(rx) < FRAME_BYTES:
        return None, elapsed_ms
    # Extract first 32 bytes — the real shield output (rest is padding zeros)
    return unpack_vector(rx[:FRAME_BYTES]), elapsed_ms


def prime_same_vector(dev, x_t, prime_frames, timeout_ms=2000):
    """
    Send the same vector a number of times before the measured transaction.

    This is a practical host-side workaround for FX2 firmware configurations
    where EP2 FLAGA only asserts after multiple 16-word frames have queued.
    Any responses returned during priming are intentionally discarded.
    """
    for _ in range(prime_frames):
        transact(dev, x_t, timeout_ms=timeout_ms)


def fmt(v, n=6):
    s = ", ".join(str(x) for x in v[:n])
    if len(v) > n:
        s += f", … ({len(v)-n} more)"
    return "[" + s + "]"


# ---------------------------------------------------------------------------
# Test suite
# ---------------------------------------------------------------------------
def run_tests(dev, anxiety=0, prime_frames=0):
    B = B_BASE - ((ALPHA * anxiety) >> 10)
    print(f"\n{'='*68}")
    print(f"  Aegis-Chip CC-CBF Test Harness  (USB / CY68013A FX2LP)")
    print(f"  anxiety_level = {anxiety}  ({anxiety_to_buttons(anxiety)})")
    print(f"  B = {B_BASE} − (({ALPHA}×{anxiety})>>10) = {B}")
    if prime_frames:
        print(f"  priming each case with {prime_frames} identical frame(s)")
    print(f"  W = {W[:8]}")
    print(f"      {W[8:]}")
    print(f"{'='*68}\n")

    passed = failed = timeouts = 0

    for tc in TEST_CASES:
        x_t      = tc["x_t"]
        note     = tc.get("note", "")
        expected, h = reference_cbf(x_t, anxiety)

        if prime_frames:
            prime_same_vector(dev, x_t, prime_frames)
        received, ms = transact(dev, x_t)

        if received is None:
            status = FAIL
            detail = f"{RED}TIMEOUT / no response{RST}"
            timeouts += 1
        elif received == expected:
            status = PASS
            detail = f"u_t = {fmt(received)}"
            passed += 1
        else:
            status = FAIL
            detail = (f"got {RED}{fmt(received)}{RST}\n"
                      f"           exp {GRN}{fmt(expected)}{RST}")
            failed += 1

        print(f"[{status}] {tc['name']}")
        if note:
            print(f"           {YLW}{note}{RST}")
        print(f"           x_t = {fmt(x_t)}   h = {h}")
        print(f"           {detail}   ({ms:.1f} ms)\n")

    print("="*68)
    print(f"Results: {GRN}{passed} passed{RST}, {RED}{failed} failed{RST}, "
          f"{YLW}{timeouts} timeouts{RST}  / {len(TEST_CASES)} total")

    if failed == 0 and timeouts == 0:
        print(f"\n{GRN}✓ All tests passed — CC-CBF USB transport verified.{RST}")
    else:
        print(f"\n{RED}✗ Tests failed. Diagnostics:{RST}")
        if timeouts > 0:
            print("  Timeouts:")
            print("    • Is the FPGA programmed?  ls -lh impl/aegis_chip_usb.bit")
            print("    • Is the CY68013A firmware configuring EP2/EP6 in slave-FIFO mode?")
            print("    • Is usb_flaga asserted after you send data?")
            print("      (Logic analyser on usb_flaga/slrd pins is helpful here)")
        if failed > 0:
            print("  Wrong values:")
            print("    • Anxiety level: does board button state match --anxiety arg?")
            print("    • Byte order: check little-endian, LSB first per word")
    print()


# ---------------------------------------------------------------------------
# Interactive mode
# ---------------------------------------------------------------------------
def run_interactive(dev, anxiety=0, prime_frames=0):
    B = B_BASE - ((ALPHA * anxiety) >> 10)
    print(f"\n{YLW}Interactive mode{RST}  (type 'quit' to exit)")
    print(f"Enter 16 comma-separated INT16 values for x_t.")
    print(f"anxiety_level={anxiety}  ({anxiety_to_buttons(anxiety)}),  B={B}\n")
    if prime_frames:
        print(f"Priming each entered vector with {prime_frames} identical frame(s).\n")

    while True:
        try:
            raw = input("x_t> ").strip()
        except (EOFError, KeyboardInterrupt):
            break
        if raw.lower() in ("q", "quit", "exit"):
            break
        try:
            vals = [int(v.strip()) for v in raw.split(",")]
            if len(vals) != FRAME_WORDS:
                print(f"  Need {FRAME_WORDS} values, got {len(vals)}")
                continue
            if not all(-32768 <= v <= 32767 for v in vals):
                print("  Values must be [-32768, 32767]")
                continue
        except ValueError:
            print("  Invalid input")
            continue

        expected, h = reference_cbf(vals, anxiety)
        if prime_frames:
            prime_same_vector(dev, vals, prime_frames)
        received, ms = transact(dev, vals)

        print(f"  h          = {h}")
        print(f"  expected   = {expected}")
        if received is None:
            print(f"  received   = {RED}TIMEOUT{RST}")
        elif received == expected:
            print(f"  received   = {GRN}{received}{RST}  ({ms:.1f} ms)  {PASS}")
        else:
            print(f"  received   = {RED}{received}{RST}  ({ms:.1f} ms)  {FAIL}")
        print()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Aegis-Chip USB test harness (CY68013A FX2LP)")
    parser.add_argument("--vid", type=lambda x: int(x, 0),
                        help="USB VID override (e.g. 0x04B4)")
    parser.add_argument("--pid", type=lambda x: int(x, 0),
                        help="USB PID override (e.g. 0x00F1)")
    parser.add_argument("--scan-devices", action="store_true",
                        help="List all connected USB devices and exit")
    parser.add_argument("--anxiety", type=int, default=0,
                        metavar="LEVEL",
                        help="Anxiety level matching board button state (0–960, ×64)")
    parser.add_argument("--interactive", action="store_true",
                        help="Enter custom x_t vectors interactively")
    parser.add_argument("--prime-frames", type=int, default=0,
                        metavar="N",
                        help="Send N identical warm-up frames before each measured transaction")
    parser.add_argument("--single",
                        help='Run one vector, e.g. --single "-100,-100,...,-100"')
    args = parser.parse_args()

    if args.prime_frames < 0:
        print("--prime-frames must be >= 0")
        sys.exit(1)

    if args.scan_devices:
        scan_devices()
        sys.exit(0)

    dev = open_device(vid=args.vid, pid=args.pid)

    try:
        if args.interactive:
            run_interactive(dev, anxiety=args.anxiety, prime_frames=args.prime_frames)
        elif args.single:
            vals = [int(v.strip()) for v in args.single.split(",")]
            if len(vals) != FRAME_WORDS:
                print(f"Need {FRAME_WORDS} values, got {len(vals)}")
                sys.exit(1)
            expected, h = reference_cbf(vals, args.anxiety)
            if args.prime_frames:
                prime_same_vector(dev, vals, args.prime_frames)
            received, ms = transact(dev, vals)
            print(f"anxiety  = {args.anxiety}  ({anxiety_to_buttons(args.anxiety)})")
            if args.prime_frames:
                print(f"primed    = {args.prime_frames} identical frame(s)")
            print(f"x_t      = {vals}")
            print(f"h        = {h}")
            print(f"expected = {expected}")
            print(f"received = {received}  ({ms:.1f} ms)")
            ok = received == expected
            print(f"result   = {PASS if ok else FAIL}")
            sys.exit(0 if ok else 1)
        else:
            run_tests(dev, anxiety=args.anxiety, prime_frames=args.prime_frames)
    except KeyboardInterrupt:
        print("\nInterrupted.")

if __name__ == "__main__":
    main()
