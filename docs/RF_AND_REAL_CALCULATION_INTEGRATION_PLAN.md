# RF and Real Calculation Integration Plan

## Goal

Restore the intended end-to-end Aegis data path in controlled stages:

- real steering-vector calculation in `aegis_shield`
- real anxiety estimation from `rf_anxiety`
- eventually real biosignal feature inputs from ECG/EDA DSP blocks
- while keeping USB bring-up debuggable and avoiding mixed-failure ambiguity

The key principle is:

> integrate **one risk at a time**.

Right now the project has three mostly separate concerns:

1. **USB transport correctness**
2. **real CC-CBF / steering-vector math correctness**
3. **RF anxiety inference correctness**

Those should not be reintroduced simultaneously.

## Current verified state

### Working / present

- `test_aegis_usb.py` supports `--prime-frames N`
- USB path is practically usable with `--prime-frames >= 2`
- `rtl/aegis_usb_bridge.v` has already received:
  - EP6 pacing fix
  - timer-width fix
- RF HLS source exists:
  - `hls/rf_anxiety/rf_anxiety.cpp`
- generated RF IP exists:
  - `ip_repo/rf_anxiety_ip/hdl/verilog/rf_anxiety.v`
- real shield HLS source exists:
  - `hls/aegis_shield/aegis_shield.cpp`
  - `hls/aegis_shield/aegis_shield.h`

### Not yet active in the USB top

- active `rtl/aegis_shield.v` is still a **debug stub**
- active `rtl/aegis_top_usb.v` still drives `anx_in` from `btn_anxiety`
- active USB build does **not** yet instantiate the real RF path
- active USB build does **not** yet connect real ECG/EDA DSP outputs into RF

## Integration strategy

Use five technical checkpoints plus one cleanup phase.

---

## Checkpoint 1 — Restore real steering-vector math only

### Objective

Replace the debug stub in `rtl/aegis_shield.v` with the real steering-vector implementation while **keeping anxiety input simple**.

### Recommended approach

- keep `btn_anxiety` as the source of `anxiety_level`
- restore the real `aegis_shield` implementation from the HLS-generated IP or a clean RTL wrapper around it
- do **not** integrate RF yet
- do **not** integrate ECG/EDA DSP yet

### Why this comes first

This isolates the question:

> “Does USB + frame handling + real CC-CBF math work?”

without adding uncertainty from RF feature generation.

### Deliverables

- USB top returns actual computed `u_t`, not constant debug patterns
- existing host harness can compare hardware result against the software reference model
- a small set of known vectors pass reproducibly using `--prime-frames 2`

### Exit criteria

The following should pass repeatedly:

- zero vector
- all `+1`
- all `-2`
- all `-100`
- one asymmetric test like `x[0] = -500`

for at least 2–3 button-selected anxiety levels.

---

## Checkpoint 2 — Lock down math validation over the current USB path

### Objective

Prove the real shield math is correct enough before adding RF.

### Recommended test additions

Extend `test_aegis_usb.py` or create a companion regression script that includes:

- deterministic non-zero vectors that the debug stub could never pass accidentally
- multiple anxiety levels
- repeated primed runs on one open device handle
- optional summary table of pass/fail rate

### Suggested cases

- safe case: `x_t = [0]*16`
- shallow violation: `[-2]*16`
- large violation: `[-100]*16`
- asymmetric case: `[-500, 0, ..., 0]`
- mixed-sign case: `[10, -20] * 8`
- saturation case: `[-1000]*16`

### Exit criteria

- hardware matches software golden model for all selected cases
- behavior is stable with `--prime-frames 2`
- no evidence of frame-order or byte-order corruption

---

## Checkpoint 3 — Integrate RF inference with controlled feature inputs

### Objective

Introduce `rf_anxiety` into the active USB top while keeping the RF inputs synthetic or controlled.

### Recommended approach

- instantiate generated `rf_anxiety` IP in the USB top or in a small wrapper module
- temporarily drive RF features from:
  - fixed constants, or
  - a small test register bank / simple deterministic stimulus block
- route RF output to `anx_in` instead of `btn_anxiety`
- optionally preserve a debug compile-time switch between:
  - `btn_anxiety`
  - synthetic RF stimulus
  - real RF output

### Why this stage matters

This isolates the question:

> “Does the integrated RF block produce sensible `anxiety_level` values, and does shield respond correctly to them?”

without requiring the real biosignal pipeline yet.

### Recommended observability

Add temporary visibility for:

- RF feature inputs
- RF output `anxiety_level`
- selected source of `anxiety_level`

This can be done through:

- LEDs for coarse mode selection only
- host-readable debug words if practical
- or temporary alternate test modes

### Exit criteria

- known synthetic feature sets produce expected low/medium/high anxiety outputs
- `aegis_shield` response changes accordingly for the same `x_t`
- RF output remains numerically stable across repeated runs

---

## Checkpoint 4 — Connect real ECG/EDA DSP blocks into RF

### Objective

Replace synthetic RF features with real DSP feature outputs.

### Recommended approach

- instantiate or restore the intended ECG DSP and EDA DSP blocks
- connect their fixed-point feature outputs into `rf_anxiety`
- verify scaling, range, and update cadence carefully
- preserve a bypass mode for debug if possible

### Main risks here

- feature scaling mismatch
- timing / latency mismatch between DSP outputs and RF sampling
- stale feature windows
- overflow / truncation in fixed-point conversions
- difficult observability once everything is connected

### Recommended safeguards

- document expected range for each RF input feature
- define and freeze the UART physiology-ingest protocol before wiring the external sender to live DSP inputs
- create one debug mode exposing raw feature values over host transport or UART
- compare several captured FPGA feature sets against software-computed reference values offline

### Exit criteria

- real DSP features are numerically believable
- RF output tracks changes in physiological input in the expected direction
- no obvious clipping or constant/stuck outputs

---

## Checkpoint 5 — Full system validation

### Objective

Validate the intended chain:

$$
\text{biosignals} \rightarrow \text{DSP features} \rightarrow \text{RF anxiety} \rightarrow \text{CC-CBF steering vector}
$$

### Validation goals

- `x_t` still transports correctly over USB
- `anxiety_level` is now adaptive instead of button-forced
- `u_t` changes correctly as both `x_t` and anxiety vary
- the system remains stable over repeated transactions

### Recommended tests

- fixed `x_t`, sweep anxiety by controlled RF feature injection
- fixed anxiety-like feature set, sweep `x_t`
- repeated regression runs after reprogramming
- long-run test for several hundred or thousand transactions

### Exit criteria

- end-to-end behavior matches the intended architecture
- results are reproducible after reconfiguration and reset
- no dependence on accidental debug state

---

## Cleanup phase — Remove temporary bring-up scaffolding

### Objective

Remove or quarantine the temporary debug aids once the real pipeline is stable.

### Items to clean up

- debug-stub `rtl/aegis_shield.v`
- temporary LED debug mappings in `rtl/aegis_top_usb.v`
- temporary debug-only muxes if they are no longer needed
- host assumptions that are only valid for the stub

### USB-specific cleanup

The host-side priming workaround should remain available until the FX2 behavior is properly fixed, but should be documented as temporary.

Long-term goal:

- inspect FX2 firmware / EEPROM endpoint configuration
- verify exact `FLAGA` semantics
- eliminate the need for `--prime-frames`

## Concrete implementation order

Recommended practical order of work:

1. Restore real `aegis_shield` in the USB build.
2. Validate CC-CBF math using `btn_anxiety` and `test_aegis_usb.py --prime-frames 2`.
3. Add stronger regression vectors that cannot be confused with stub behavior.
4. Instantiate RF with synthetic feature inputs.
5. Switch `anx_in` source from buttons to RF output.
6. Validate RF-driven steering behavior with controlled stimuli.
7. Connect ECG/EDA DSP outputs to RF inputs.
8. Validate full biosignal → RF → steering chain.
9. Investigate and remove the USB priming workaround at the source.

Before step 7 is treated as complete, the UART physiology ingress contract should be written down and agreed in:

- `docs/UART_PHYSIOLOGY_PROTOCOL_SPEC.md`

## Suggested branch / milestone structure

To reduce rollback pain, use small milestones such as:

- `milestone/usb-real-shield`
- `milestone/usb-real-shield-validated`
- `milestone/rf-synthetic-inputs`
- `milestone/rf-connected-to-shield`
- `milestone/full-biosignal-chain`
- `milestone/fx2-transport-cleanup`

## Recommendation summary

The safest path is:

1. **real shield first**
2. **RF second**
3. **real DSP features third**
4. **FX2 cleanup in parallel or after functional recovery**

That order gives the best chance of always knowing which subsystem broke when something goes sideways.

In gloriously plain engineering terms:

- first make the math real,
- then make anxiety smart,
- then make the inputs real,
- then make the transport elegant.
