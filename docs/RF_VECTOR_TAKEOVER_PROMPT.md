You are taking over the restoration of the **real CC-CBF steering-vector path** and the **random-forest anxiety path** in the `aegis_fpga` repository.

Your goal is to continue implementation safely and incrementally, without mixing transport, math, and physiology-ingest risks all at once.

## What you must understand first

The repository contains both:

1. the **target design**, and
2. the **current active checked-in implementation**.

These are **not the same**.

### Current active checked-in implementation

As of 2026-03-15:

- active top: `rtl/aegis_top_usb.v`
- active build script: `scripts/build_vivado_usb.tcl`
- active bitstream: `impl/aegis_chip_usb.bit`
- active host test: `test_aegis_usb.py`
- current transport: **USB via CY68013A FX2LP slave FIFO**
- current `anxiety_level` source: `rtl/btn_anxiety.v`
- current `rtl/aegis_shield.v`: **debug stub**, not the real steering-vector engine
- current USB transport is usable with `--prime-frames >= 2`

### Planned future design intent

The intended restored system is:

- external device sends **ECG + EDA** to the board over **UART**
- FPGA ingests UART physiology data
- ECG/EDA DSP blocks compute 12 RF features
- `rf_anxiety` produces adaptive `anxiety_level`
- real `aegis_shield` computes steering vector `u_t`
- host still interacts with the FPGA through the active transport/test path during bring-up

## Critical traps

1. **Do not assume the active USB build already uses the real shield or RF.**
   - It does not.
2. **Do not assume rebuilding HLS IP changes the USB bitstream by itself.**
   - `scripts/build_all_hls.tcl` regenerates IP in `ip_repo/`, but `scripts/build_vivado_usb.tcl` builds from `rtl/*.v` and `aegis_top_usb`.
3. **Do not try to restore USB transport, real shield math, RF, and UART physiology ingest all at once.**
   - Integrate one risk at a time.
4. **Do not assume the ECG/EDA feature normalization constants are final.**
   - Verify them against the training pipeline.

## Read these files first

1. `docs/ARCHITECTURE.md`
2. `docs/BURN_AND_TEST.md`
3. `docs/debug/USB_DEBUG_STATUS_2026-03-15.md`
4. `docs/RF_AND_REAL_CALCULATION_INTEGRATION_PLAN.md`
5. `docs/RF_VECTOR_IMPLEMENTATION_HANDOVER.md`
6. `docs/UART_PHYSIOLOGY_PROTOCOL_SPEC.md`
7. `rtl/aegis_top_usb.v`
8. `rtl/aegis_shield.v`
9. `hls/aegis_shield/aegis_shield.cpp`
10. `hls/rf_anxiety/rf_anxiety.cpp`
11. `rtl/uart_rx.v`
12. `hls/ecg_dsp/ecg_dsp.h`
13. `hls/eda_dsp/eda_dsp.h`

## Your staged mission

### Phase 1 — restore real shield only

Replace the debug stub path with the real `aegis_shield` behavior while **keeping**:

- USB transport as-is
- `btn_anxiety` as the temporary `anxiety_level` source

Success criteria:

- `test_aegis_usb.py --prime-frames 2` matches the software golden model
- non-zero violation cases work correctly
- no confusion with stub behavior remains

### Phase 2 — strengthen regression

Use or extend `test_aegis_usb.py` to validate at least:

- `[0]*16`
- `[1]*16`
- `[-2]*16`
- `[-100]*16`
- `[-500,0,...,0]`
- `[10,-20]*8`
- `[-1000]*16`

across multiple anxiety settings.

### Phase 3 — integrate RF with synthetic inputs first

Instantiate `rf_anxiety` with controlled feature inputs before using real ECG/EDA data.

Success criteria:

- low/medium/high synthetic feature sets produce expected anxiety outputs
- the same `x_t` produces different steering behavior when RF output changes

### Phase 4 — finalize UART physiology protocol

Before wiring ECG/EDA into DSP and RF, complete `docs/UART_PHYSIOLOGY_PROTOCOL_SPEC.md`.

At minimum, define:

- framing model
- channel ordering
- signed sample encoding
- endianess
- resynchronization behavior
- startup/idle behavior
- how `sample_valid` is derived for ECG and EDA streams

### Phase 5 — connect real ECG/EDA DSP outputs to RF

Only after the UART protocol and synthetic RF phase are stable:

- ingest UART-fed ECG/EDA
- demultiplex channels
- feed ECG and EDA DSP blocks
- connect DSP features to `rf_anxiety`
- validate feature plausibility and normalization

## Assumptions you may use initially

- `rtl/uart_rx.v` currently receives UART 8-N-1 at 115200 baud
- two bytes are assembled into one signed 16-bit little-endian word
- a simple initial physiology convention can be:
  - word 0 = ECG
  - word 1 = EDA
  - word 2 = ECG
  - word 3 = EDA
- 700 Hz ECG + 700 Hz EDA is bandwidth-feasible at 115200 baud

## Validation priorities

When making changes, prioritize observing these separately:

1. transport correctness
2. shield math correctness
3. RF output correctness
4. ECG/EDA feature correctness
5. UART physiology ingest correctness

If something fails, identify **which layer** is failing before changing multiple subsystems.

## Deliverables expected from you

- code changes in small, reviewable steps
- updated docs whenever assumptions change
- explicit notes when a doc describes target design vs active current implementation
- regression evidence for each phase before moving to the next

## Minimal acceptable first milestone

Your first successful milestone is:

> the USB build returns the real `aegis_shield` steering vector for host-supplied `x_t`, with button-driven `anxiety_level`, and passes repeated primed regression tests.

Do **not** skip directly to full UART-fed physiology integration before this milestone is reached.

---

If you are unsure about any protocol or integration contract, update the corresponding doc first instead of letting the implementation implicitly define it.
