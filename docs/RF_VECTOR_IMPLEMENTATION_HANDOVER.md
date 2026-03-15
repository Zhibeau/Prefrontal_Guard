# RF + Vector Calculation Takeover Handover

## Purpose

This document is for the engineer taking over restoration of:

- the real steering-vector calculation (`aegis_shield`)
- the real random-forest anxiety path (`rf_anxiety`)

It focuses on what is true **right now**, what needs to be changed, and what pitfalls are easy to miss.

## Executive summary

The repository already contains the algorithmic source code for both major compute blocks:

- `hls/aegis_shield/*` — real CC-CBF steering-vector calculation
- `hls/rf_anxiety/*` — real RF anxiety inference

However, the active USB image does **not** currently use them.

Today’s active USB build instead uses:

- `rtl/aegis_top_usb.v`
- `rtl/aegis_shield.v` (**debug stub**)
- `rtl/btn_anxiety.v` for `anxiety_level`

So the first job is not to “improve the algorithm,” but to **restore the intended hardware integration path**.

---

## What is active now

### Active top and build

- top module: `rtl/aegis_top_usb.v`
- build script: `scripts/build_vivado_usb.tcl`
- bitstream: `impl/aegis_chip_usb.bit`

### Active transport

- USB via CY68013A FX2LP slave FIFO
- host test script: `test_aegis_usb.py`
- frame format: 16 × signed 16-bit little-endian words

### Active compute path

- `rtl/aegis_shield.v` is a debug stub returning constant vectors
- `anx_in` currently comes from `rtl/btn_anxiety.v`

### Active USB nuance

- use `--prime-frames 2` as the current known-good transport mode
- without priming, the first transactions may time out

### Planned live physiology source

For the RF integration takeover, the intended live physiology source is:

- **an external device sending ECG and EDA data into the board UART port**

This is an important architectural intent that is not yet the active checked-in integration.

So the takeover engineer should think in terms of two separate paths:

1. current active USB path for vector/transport bring-up
2. future UART physiology ingress path for live RF input

---

## What source code already exists for the real implementation

### Real shield math

Files:

- `hls/aegis_shield/aegis_shield.h`
- `hls/aegis_shield/aegis_shield.cpp`

Important constants:

- `DIM = 16`
- `B_BASE = 5000`
- `ALPHA = 6000`
- `W = [64, 128, 96, 112, 80, 72, 104, 88, 120, 60, 92, 116, 76, 84, 100, 68]`

Important implementation note from the HLS source:

- the stream type is deliberately `hls::axis<ap_int<16>>`, not unsigned `ap_axiu<16>`
- this is to preserve sign handling for negative vector elements

### Real RF inference

Files:

- `hls/rf_anxiety/rf_anxiety.h`
- `hls/rf_anxiety/rf_anxiety.cpp`
- `hls/rf_anxiety/rf_biological_arousal_fixed.h`

Key facts:

- 12 feature inputs
- fixed-point implementation
- output target range `0..1024`

### DSP feature contract caveat

The eventual RF inputs are expected to come from:

- `hls/ecg_dsp/ecg_dsp.h`
- `hls/eda_dsp/eda_dsp.h`

Those headers already document the intended feature list, but they also explicitly warn that the z-score constants should be replaced with the exact scaler statistics from the training pipeline.

Implication:

- restoring RF wiring is feasible now,
- but the final DSP → RF hookup should not be considered statistically correct until those normalization constants are verified against the training artifacts.

### Generated HLS IP already present

Generated IP directories already exist in `ip_repo/`:

- `ip_repo/aegis_shield_ip/`
- `ip_repo/rf_anxiety_ip/`
- `ip_repo/ecg_dsp_ip/`
- `ip_repo/eda_dsp_ip/`

---

## Most important trap in the repository

### Rebuilding HLS IP is not enough

`scripts/build_all_hls.tcl` regenerates IP into `ip_repo/`, but the active USB Vivado script:

- `scripts/build_vivado_usb.tcl`

currently adds:

- `rtl/*.v`

and sets top to:

- `aegis_top_usb`

This means:

> you can rebuild HLS IP successfully and still get a hardware image that uses the stub.

If you want the real shield or RF in the USB build, you must explicitly change the active RTL/build integration.

---

## Required implementation decisions

The takeover engineer should decide one of these integration styles for each block:

### Option A — instantiate generated HLS IP directly

Pros:

- matches generated IP exactly
- lower risk of drift from HLS output

Cons:

- requires wiring the IP into the Vivado project and active top cleanly

### Option B — replace the stub with a generated-Verilog drop-in wrapper

Pros:

- can preserve the current simple module boundary used by `rtl/aegis_top_usb.v`

Cons:

- requires care to keep interfaces aligned with the top-level wiring

### Recommendation

For the first restoration step, prefer the smallest change that replaces the stub while preserving the current USB top shape.

---

## Recommended order of implementation

### Phase 1 — restore real shield only

Goal:

- keep `btn_anxiety`
- replace stub `rtl/aegis_shield.v` with real shield behavior

Why:

- isolates transport + vector-math correctness
- avoids mixing RF issues into the first debug cycle

Validation:

- use `python3 test_aegis_usb.py --prime-frames 2`
- compare against the software golden model already in the script

### Phase 2 — harden math regression

Add or emphasize cases that the stub cannot fake:

- `[-2]*16`
- `[-100]*16`
- `[-500,0,...,0]`
- `[10,-20]*8`
- `[-1000]*16`

### Phase 3 — wire in RF with controlled features first

Goal:

- instantiate `rf_anxiety`
- feed it synthetic or fixed test features first
- route its output into `anx_in`

Why:

- isolates RF block integration from DSP feature generation

### Phase 4 — connect real ECG/EDA DSP outputs

Goal:

- feed the actual 12 features into `rf_anxiety`

Why last:

- highest integration complexity
- hardest to observe if done too early
- requires confirmation of feature normalization constants, not just signal wiring

---

## Interface facts you will need

### Host vector interface

From `test_aegis_usb.py`:

- input vector length = 16
- element type = signed 16-bit
- byte order = little-endian
- host writes 32 payload bytes to EP2
- host reads a 512-byte packet from EP6 and uses the first 32 bytes as output

### Planned UART physiology ingress

From `rtl/uart_rx.v`:

- UART is currently 8-N-1 at 115200 baud by default
- two received bytes are assembled into one 16-bit little-endian word

This makes the most straightforward physiology-ingest convention:

- external sender transmits signed 16-bit ECG/EDA samples as UART words
- likely initial ordering: alternating ECG word, EDA word, ECG word, EDA word

Recommended first implementation assumption:

```text
word 0 = ECG sample
word 1 = EDA sample
word 2 = ECG sample
word 3 = EDA sample
...
```

### UART bandwidth sanity check

At 700 samples/s for ECG and 700 samples/s for EDA:

- raw payload = 22400 bit/s
- with UART 8-N-1 framing = about 28000 bit/s on the wire

So the current default 115200 baud is comfortably sufficient for a simple two-channel 16-bit sample stream.

### Missing implementation decision

The repository does **not** yet define the final UART physiology framing protocol. The takeover engineer still needs to decide:

- pure interleaved samples vs framed packets
- resynchronization strategy after byte loss
- whether to add headers / markers / channel IDs
- how `sample_valid` is generated toward ECG/EDA DSP blocks

### Shield interface shape in the active top

From `rtl/aegis_top_usb.v`:

- `anx_in` is 16-bit
- `x_t_*` and `u_t_*` use AXI-Stream style signals
- `u_t` result is sent back out through the same USB bridge path

### Anxiety-level range nuance

Current button path:

- `0..960` in steps of `64`

RF path target:

- `0..1024`

Implication:

- when RF is integrated, the host test harness may need a small documentation or CLI update so it no longer implies button-only range limits

---

## Known transport constraints during bring-up

Use these assumptions while restoring functionality:

- the USB bridge is currently usable with `--prime-frames >= 2`
- do not treat removal of priming as a prerequisite for restoring real compute
- first recover functional math
- then clean up transport behavior later

Relevant note:

- `docs/debug/USB_DEBUG_STATUS_2026-03-15.md`

---

## Suggested validation checklist

### After restoring real shield

- [ ] zero vector returns all zeros for low anxiety
- [ ] negative vectors produce non-zero steering where expected
- [ ] asymmetric test weights behave as expected
- [ ] saturation case clamps to `[-32768, 32767]`
- [ ] repeated primed runs behave consistently

### After integrating RF with synthetic features

- [ ] low-feature stimulus gives low anxiety
- [ ] high-feature stimulus gives high anxiety
- [ ] same `x_t` with different RF outputs changes `u_t` as expected

### After integrating real DSP features

- [ ] all 12 RF inputs are numerically sensible
- [ ] no stuck-at-zero or stuck-at-constant features
- [ ] changing physiology surrogate inputs changes RF output in the expected direction
- [ ] ECG/EDA z-score normalization constants are verified against the training pipeline, not just assumed from placeholder values

---

## Files most likely to be edited during takeover

- `rtl/aegis_top_usb.v`
- `rtl/aegis_shield.v` or its replacement/wrapper
- `scripts/build_vivado_usb.tcl`
- possibly a new wrapper module for generated HLS IP
- `test_aegis_usb.py` for RF-aware validation improvements

Potentially later:

- `rtl/adc_frontend.v`
- UART-based ECG/EDA ingest wrapper / demultiplexer
- ECG/EDA DSP integration wiring

---

## Minimal success definition

The first successful handover milestone is **not** full physiology integration.

It is this:

> The USB build returns the real `aegis_shield` steering vector for host-supplied `x_t`, with button-driven `anxiety_level`, and passes repeated primed regression tests.

Once that milestone is reached, RF integration becomes much safer and much more interpretable.

## Read this next

1. `docs/ARCHITECTURE.md`
2. `docs/BURN_AND_TEST.md`
3. `docs/debug/USB_DEBUG_STATUS_2026-03-15.md`
4. `docs/RF_AND_REAL_CALCULATION_INTEGRATION_PLAN.md`