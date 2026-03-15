# Aegis-Chip Architecture

## Status at a glance

This document describes **both**:

1. the **target architecture** of the project, and
2. the **current checked-in USB implementation** in this repository.

These are **not the same today**.

## Current checked-in implementation (important)

As of 2026-03-15, the active, buildable USB path is:

```text
Host PC
        ↕ USB bulk (CY68013A FX2LP slave FIFO)
aegis_usb_bridge
        ↕ AXI-Stream
aegis_uart_bridge
        ↕ AXI-Stream
rtl/aegis_shield.v   (DEBUG STUB, not real CC-CBF math)
```

### Active build facts

- Active top: `rtl/aegis_top_usb.v`
- Active Vivado build script: `scripts/build_vivado_usb.tcl`
- Active bitstream output: `impl/aegis_chip_usb.bit`
- Active test harness: `test_aegis_usb.py`
- Current transport: **USB**, not UART
- Current `anxiety_level` source: `rtl/btn_anxiety.v`
- Current `aegis_shield`: **debug stub**, not the real HLS/CC-CBF engine

### What is not active yet

- `hls/aegis_shield/` real steering-vector implementation exists, but is not the active compute block in the USB build
- `hls/rf_anxiety/` real RF inference implementation exists, but is not wired into `rtl/aegis_top_usb.v`
- ECG/EDA DSP blocks are not wired into the active USB path

## Target architecture (intended end state)

The intended end-to-end architecture remains:

```text
ECG ADC / EDA ADC
                → ECG DSP / EDA DSP feature extraction
                → RF anxiety inference
                → adaptive anxiety_level
                → CC-CBF steering-vector calculation
                → corrected vector back to host
```

That target is still technically consistent with the codebase, but it is **not** the current checked-in hardware integration.

### Planned physiology ingress path for RF work

For the upcoming RF restoration work, the intended source of physiological input is:

```text
external device
        → board UART port
        → UART RX / ingest logic on FPGA
        → ECG/EDA sample streams
        → ECG DSP / EDA DSP feature extraction
        → rf_anxiety
```

In other words:

- the FPGA is expected to receive **ECG and EDA data over the board UART port** from another device;
- those UART-fed samples are intended to become the live inputs to the DSP blocks and then the RF model;
- this UART-fed physiology path is **planned / intended**, but is **not yet the active checked-in USB build**.

This distinction is important for handover:

- current active hardware path = USB vector test path with stub shield;
- intended future physiology path = UART-fed ECG/EDA into RF.

---

## Current USB data path

### Top-level module

`rtl/aegis_top_usb.v`

Pipeline:

```text
USB EP2 OUT (host → FPGA)
        → aegis_usb_bridge
        → aegis_uart_bridge
        → aegis_shield   (currently stub)
        → aegis_usb_bridge
        → USB EP6 IN (FPGA → host)
```

### Frame format

- 16 words per frame
- each word is signed 16-bit little-endian
- host sends 32 bytes per request
- FPGA replies with a 512-byte USB packet whose first 32 bytes contain the real result and the rest are padding zeros

### Current anxiety source

`rtl/btn_anxiety.v`

- 4 debounced board buttons
- encoded in steps of 64
- practical range in the current button flow: `0..960`

### Current compute block

`rtl/aegis_shield.v`

This is currently a **debug loopback/stub**.

Behavior:

- drains 16 input words
- latches `anx_in[9:8]`
- emits one of four constant 16-word patterns

So the active USB build currently validates **transport and control plumbing**, not the real steering-vector math.

---

## Real compute blocks that exist in source

### Real CC-CBF / steering-vector engine

Files:

- `hls/aegis_shield/aegis_shield.cpp`
- `hls/aegis_shield/aegis_shield.h`

Implemented math:

$$
B = B_{base} - \left(\frac{\alpha \cdot anxiety}{2^{10}}\right)
$$

$$
h = B + W \cdot x_t
$$

If $h \ge 0$:

$$
u_t = 0
$$

If $h < 0$:

$$
u_t[i] = clamp_{16}\left(\frac{-h \cdot W[i]}{2^{10}}\right)
$$

Constants from `hls/aegis_shield/aegis_shield.h`:

- `DIM = 16`
- `B_BASE = 5000`
- `ALPHA = 6000`
- `W = [64, 128, 96, 112, 80, 72, 104, 88, 120, 60, 92, 116, 76, 84, 100, 68]`

### Real RF inference engine

Files:

- `hls/rf_anxiety/rf_anxiety.cpp`
- `hls/rf_anxiety/rf_anxiety.h`
- `hls/rf_anxiety/rf_biological_arousal_fixed.h`

Function:

- consumes 12 fixed-point features
- runs a 30-tree random forest
- outputs `anxiety_level` in range `0..1024`

Inputs:

1. `feat_ecg_hr`
2. `feat_ecg_std`
3. `feat_ecg_rmssd`
4. `feat_ecg_pnn50`
5. `feat_ecg_sdnn`
6. `feat_eda_mean`
7. `feat_eda_std`
8. `feat_eda_min`
9. `feat_eda_max`
10. `feat_eda_scl`
11. `feat_eda_scr`
12. `feat_eda_peaks`

---

## HLS/IP generation versus active USB build

This is the most important handover warning in the repository.

### What `scripts/build_all_hls.tcl` does

It synthesizes and exports these IPs into `ip_repo/`:

- `ecg_dsp_ip`
- `eda_dsp_ip`
- `rf_anxiety_ip`
- `aegis_shield_ip`

### What `scripts/build_vivado_usb.tcl` does

It creates a Vivado project for:

- top = `aegis_top_usb`
- RTL sources = `rtl/*.v`
- constraints = `constraints/aegis_ax7102_usb.xdc`

### Critical implication

Rebuilding HLS IP alone does **not** change the current USB hardware image unless the teammate also:

- instantiates the generated IP or equivalent wrapper in the active top, and/or
- modifies the RTL/build flow to include and use that IP.

Today, the active USB build is driven by `rtl/*.v`, and `rtl/aegis_shield.v` is still the stub.

---

## Current USB transport nuance

The USB path is usable, but not yet elegant.

Observed and documented behavior:

- unprimed first transaction often times out
- `--prime-frames >= 2` makes measured transfers reliable
- likely root cause is FX2 endpoint/flag threshold behavior

Relevant docs:

- `docs/debug/USB_DEBUG_STATUS_2026-03-15.md`
- `docs/RF_AND_REAL_CALCULATION_INTEGRATION_PLAN.md`

## Accuracy notes for older mental models

The following statements are **not accurate** for the current checked-in USB build:

- “The active top is `rtl/aegis_top.v`.”
- “The active transport path is UART.”
- “The active hardware already includes ECG/EDA DSP feeding RF.”
- “The active `aegis_shield` is the real CC-CBF engine.”
- “Rebuilding HLS IPs automatically changes the USB bitstream.”

Those describe the target system or older design intent, not the current implementation.

## Planned UART physiology data path details

The repository already contains the basic UART byte/word building blocks:

- `rtl/uart_rx.v`
- `rtl/uart_tx.v`

Current `uart_rx.v` behavior:

- receives 8-N-1 UART at 115200 baud by default
- assembles **two bytes** into **one 16-bit little-endian word**

That means an external device can already target a simple word-stream interface where:

- first byte = low byte
- second byte = high byte
- resulting word = signed 16-bit sample

### Recommended future sample ordering

If ECG and EDA are streamed over one UART link from another device, the simplest initial convention is:

```text
ECG sample word
EDA sample word
ECG sample word
EDA sample word
...
```

with each sample encoded as one 16-bit little-endian signed word.

This is not yet implemented as the active physiology ingest protocol, but it is the most natural starting point given the current `uart_rx.v` behavior.

### Throughput sanity check

If both ECG and EDA are sent at 700 samples/s and each sample is 16 bits:

$$
2 \text{ channels} \times 700 \text{ samples/s} \times 16 \text{ bits} = 22400 \text{ bit/s raw payload}
$$

UART 8-N-1 sends 10 serial bits per payload byte, so one 16-bit sample consumes 20 serial bits. Two channels at 700 samples/s gives:

$$
2 \times 700 \times 20 = 28000 \text{ bit/s on the wire}
$$

This is below 115200 baud, so the planned UART-fed physiology stream is feasible at that rate, assuming framing stays simple.

### What is still undocumented outside this note

The following still need explicit implementation when the UART physiology path is restored:

- exact ECG/EDA framing convention
- whether samples are interleaved or packetized
- reset/re-sync behavior on byte loss
- whether sample-valid strobes are derived per received word or per channel pair
- whether any marker/header bytes are used

For takeover purposes, the key point is now documented:

> the planned live RF input source is **UART-fed ECG/EDA data from another device**, not on-board USB host vectors.

## Recommended reading order for a new contributor

1. `docs/debug/USB_DEBUG_STATUS_2026-03-15.md`
2. `docs/RF_AND_REAL_CALCULATION_INTEGRATION_PLAN.md`
3. `docs/RF_VECTOR_IMPLEMENTATION_HANDOVER.md`
4. `rtl/aegis_top_usb.v`
5. `rtl/aegis_shield.v`
6. `hls/aegis_shield/aegis_shield.cpp`
7. `hls/rf_anxiety/rf_anxiety.cpp`
