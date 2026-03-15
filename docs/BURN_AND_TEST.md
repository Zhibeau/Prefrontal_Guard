# Aegis-Chip Burn & Test Guide

## Status and scope

This guide describes the **current checked-in USB build**, not the original UART-only plan.

As of 2026-03-15:

- active top = `rtl/aegis_top_usb.v`
- active bitstream = `impl/aegis_chip_usb.bit`
- active host test = `test_aegis_usb.py`
- current `aegis_shield` in the USB build = **debug stub**

That means this guide is currently best for:

- programming the board
- validating the USB transport path
- validating button-to-`anxiety_level` plumbing
- supporting future bring-up of the real shield and RF path

It does **not** yet prove that the real CC-CBF math or RF inference is active in hardware.

---

## Prerequisites

### Hardware
| Item | Notes |
|------|-------|
| AX7102 board | Powered via board supply / USB as appropriate |
| JTAG USB cable | Used for programming via Vivado |
| CY68013A USB connection | Used for host ↔ FPGA USB bulk transfer |
| Linux PC | Used for Vivado + Python USB testing |

### Software
```bash
# Vivado 2025.2 (already installed)
source /home/midu/vivado/2025.2/Vivado/settings64.sh

# Python USB package
pip install pyusb
```

---

## Part 1 — Build and program the active USB bitstream

### 1.1 Build
```bash
/home/midu/vivado/2025.2/Vivado/bin/vivado -mode batch -source scripts/build_vivado_usb.tcl
```

Expected output artifact:

```bash
ls -lh impl/aegis_chip_usb.bit
```

### 1.2 Program the FPGA

Use the scripted programming flow if available:

```bash
/home/midu/vivado/2025.2/Vivado/bin/vivado -mode batch -source scripts/program_fpga_usb.tcl
```

Or program manually through Vivado Hardware Manager.

### 1.3 Verify programming

- board powers on
- FPGA programs successfully
- USB device enumerates

---

## Part 2 — Understand the current button behavior

### Current role of the buttons

In the active USB build, the buttons still drive `anxiety_level` through `rtl/btn_anxiety.v`.

Encoding:

```text
KEY1 KEY2 KEY3 KEY4  →  anxiety_level in steps of 64
```

Range currently reachable from buttons:

- `0..960`

Examples:

- no keys → `0`
- KEY1 only → `512`
- all 4 keys → `960`

### Important caveat

Because the active `rtl/aegis_shield.v` is a debug stub, the buttons do **not** currently exercise the real barrier-math path. Instead, they select among a few constant response patterns via `anx_in[9:8]`.

---

## Part 3 — Run the current USB tests

### 3.1 Basic device check

```bash
python3 test_aegis_usb.py --scan-devices
```

### 3.2 Recommended single-vector smoke test

Use priming:

```bash
python3 test_aegis_usb.py --single "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0" --prime-frames 2
```

### 3.3 Full regression-style run

```bash
python3 test_aegis_usb.py --prime-frames 2
```

### 3.4 Interactive testing

```bash
python3 test_aegis_usb.py --interactive --prime-frames 2
```

---

## Part 4 — Understand the current USB limitation

### Priming requirement

The checked-in USB path currently behaves reliably when:

```text
--prime-frames >= 2
```

This is a known and documented workaround, not the desired final behavior.

Likely cause:

- FX2 endpoint / `FLAGA` threshold behavior

See:

- `docs/debug/USB_DEBUG_STATUS_2026-03-15.md`

### What priming means

If you use:

```bash
--prime-frames 2
```

then the host sends two identical warm-up frames and discards their responses before the measured transaction.

---

## Part 5 — What current PASS results do and do not mean

### What a PASS currently means

- USB enumeration works
- host-to-FPGA frame transport works well enough with priming
- FPGA-to-host return path works well enough with priming
- button-driven `anx_in` plumbing is alive

### What a PASS currently does **not** mean

- the real `hls/aegis_shield` CC-CBF engine is active
- the RF path is integrated
- ECG/EDA DSP features are live
- the final architecture has been restored

Because the active `rtl/aegis_shield.v` is a stub, some test cases can appear to “work” while still not exercising the real intended math.

---

## Part 6 — Build and implementation references for the takeover engineer

### Active files to inspect first

- `rtl/aegis_top_usb.v`
- `rtl/aegis_usb_bridge.v`
- `rtl/aegis_uart_bridge.v`
- `rtl/aegis_shield.v`
- `test_aegis_usb.py`

### Real algorithm sources to restore later

- `hls/aegis_shield/aegis_shield.cpp`
- `hls/aegis_shield/aegis_shield.h`
- `hls/rf_anxiety/rf_anxiety.cpp`
- `hls/rf_anxiety/rf_anxiety.h`

### Important build scripts

- `scripts/build_vivado_usb.tcl` — builds the current USB top
- `scripts/build_all_hls.tcl` — regenerates HLS IP into `ip_repo/`

### Critical warning

Re-running `scripts/build_all_hls.tcl` alone does **not** modify the active USB hardware image unless the generated IP is actually wired into the active USB top/build flow.

---

## Part 7 — Recommended current workflow for RF/vector takeover

1. Program `impl/aegis_chip_usb.bit`
2. Use `test_aegis_usb.py --prime-frames 2` as the current known-good transport mode
3. Restore the real `aegis_shield` path first
4. Validate shield math with button-driven anxiety
5. Integrate RF using synthetic/controlled feature inputs
6. Only then connect real DSP feature pipelines

For the detailed staged plan, see:

- `docs/RF_AND_REAL_CALCULATION_INTEGRATION_PLAN.md`
- `docs/RF_VECTOR_IMPLEMENTATION_HANDOVER.md`

---

## Quick reference

```bash
# Build the current USB bitstream
/home/midu/vivado/2025.2/Vivado/bin/vivado -mode batch -source scripts/build_vivado_usb.tcl

# Program the FPGA
/home/midu/vivado/2025.2/Vivado/bin/vivado -mode batch -source scripts/program_fpga_usb.tcl

# Scan USB devices
python3 test_aegis_usb.py --scan-devices

# Recommended smoke test
python3 test_aegis_usb.py --single "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0" --prime-frames 2

# Recommended regression mode
python3 test_aegis_usb.py --prime-frames 2
```
