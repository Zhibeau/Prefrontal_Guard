# USB bring-up status — 2026-03-15

## Summary

The active USB build is now usable with a host-side priming workaround.

Observed behavior is:

- a single unprimed transaction often times out;
- with repeated transactions on one open device handle, the pattern was approximately:
  - first request: timeout
  - second request: timeout
  - third and later requests: success;
- explicitly sending at least **2 warm-up frames** before the measured frame makes the measured transfer succeed reliably.

This is consistent with the current working theory that the FX2LP EP2 `FLAGA` behavior is acting like a **watermark / queued-data threshold** rather than asserting as soon as one 16-word frame arrives.

In other words:

- `--prime-frames 2` means **2 discarded frames + 1 measured frame = 3 total frames queued/observed**;
- the fact that `--prime-frames >= 2` works, while smaller values are unreliable, matches the earlier measured pattern of:
  - `None, None, success, success, success`.

This does **not** prove the exact FX2 internal threshold setting, but it is the best explanation currently supported by measurements.

## Important current-state caveat

The active compute block in the USB build is still:

- `rtl/aegis_shield.v` = **debug stub**, not the real CC-CBF implementation.

The current stub drains 16 input words and returns one of four constant vectors selected from button state (`anx_in[9:8]`). Therefore:

- current USB validation is mainly validating the **transport path**;
- passing zero-vector tests does **not** mean the real steering-vector math is already restored.

## Concrete fixes already made

### 1. USB bridge EP6 pacing fix

`rtl/aegis_usb_bridge.v` was updated so the EP6 write path re-checks `usb_flagc` between words and releases the shared data bus while waiting.

This improved correctness of the FPGA-to-host write side and removed one obvious backpressure bug.

### 2. USB bridge timer-width fix

`rtl/aegis_usb_bridge.v` originally used:

- `reg [4:0] timer`

while the top-level configured pulse/setup/hold counts up to 35 cycles.

That meant the timer could not represent the full programmed delay values. It was widened to 8 bits.

This was a real RTL bug and needed fixing regardless of the remaining FX2 behavior.

### 3. Host-side priming support

`test_aegis_usb.py` now supports:

- `--prime-frames N`

This sends and discards `N` identical frames before each measured transaction.

Verified-good practical command:

```bash
python3 test_aegis_usb.py --single "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0" --prime-frames 2
```

Repeated primed testing on a single device handle was observed to succeed for multiple consecutive iterations with sub-millisecond response time once priming was used.

## What we think is happening

Most likely cause remaining is **FX2 endpoint/flag configuration**, not the core FPGA frame FSM.

The working hypothesis is:

1. the host writes one 32-byte frame to EP2;
2. `FLAGA` does not immediately indicate readable data to the FPGA for a single frame;
3. once enough data has accumulated (empirically about three 16-word frames total), the FPGA read path proceeds normally;
4. after that point, the pipeline can continue working if enough queued traffic remains.

This would explain all of the following:

- first transaction after reset may fail or appear inconsistent;
- later repeated transactions can start succeeding;
- `--prime-frames 2` is enough, and larger values such as 3..9 also work.

## Is `--prime-frames >= 2` what we expected?

Yes — **given the measurements we collected**, this is exactly the behavior we expected to see.

More precisely:

- we did **not** expect the transport to require priming in a final design;
- but **once the measured pattern became** `timeout, timeout, success, success, success`, the prediction was that a host-side workaround of **2 warm-up frames** should make the measured request succeed;
- that prediction is now confirmed.

So this is an **expected confirmation of the current diagnosis**, not an indication that the design is fully correct yet.

## Integration implications: RF + real CC-CBF math

Despite the USB nuance, integration of the real compute pipeline still looks feasible.

### What exists already

- `hls/rf_anxiety/rf_anxiety.cpp` and generated `ip_repo/rf_anxiety_ip/.../rf_anxiety.v` exist;
- the generated RF IP is a fixed-point design with reported HLS latency of about 12 cycles and initiation interval 1;
- `rtl/aegis_top_usb.v` already has an `anx_in` path into `aegis_shield`;
- the transport path can now be exercised reliably enough using host priming.

### What is not wired yet

- the active USB top currently drives `anx_in` from `btn_anxiety`, not from RF inference;
- the active `rtl/aegis_shield.v` is a debug stub instead of the real steering-vector implementation;
- the biosignal DSP → RF → shield chain described in `ARCHITECTURE.md` is not the active USB build today.

### Recommended integration order

1. **Restore the real `aegis_shield` math first** in the USB path.
   - Keep button-driven `anxiety_level` temporarily.
   - This isolates CC-CBF math verification from RF integration risk.

2. **Verify transport + real vector math together** using `test_aegis_usb.py --prime-frames 2`.
   - This checks that the USB path is good enough for algorithm bring-up.

3. **Instantiate RF next**.
   - First use either static/synthetic features or loop in the real DSP blocks if they are ready.
   - Then replace button-driven `anxiety_level` with RF output.

4. **Only after that**, decide whether to keep priming as a temporary host workaround or fix the FX2 firmware/EEPROM endpoint configuration properly.

### Bottom line

Yes, it is still possible to integrate the RF calculation and the real vector calculation.

The current USB issue looks like a **transport-layer nuance**, not evidence that the FPGA cannot host the intended compute pipeline. The sensible engineering approach is simply:

- restore real shield math;
- validate over primed USB;
- then wire in RF inference;
- later remove the priming workaround by fixing the FX2 configuration at the source.

## Recommended next steps

### Short-term

- treat `--prime-frames 2` as the known-good USB test mode;
- keep documenting measured behavior with one open device handle;
- avoid interpreting stub-shield PASS results as proof of CC-CBF correctness.

### Medium-term

- restore the real `aegis_shield` implementation;
- add one or two deterministic non-zero math tests that cannot pass under the stub;
- keep the priming workaround in the host harness for repeatable bring-up.

### Long-term

- inspect or replace the FX2 firmware / EEPROM endpoint configuration;
- verify the exact EP2 `FLAGA` threshold / empty-flag mode;
- remove host priming once the USB endpoint behavior is correct.