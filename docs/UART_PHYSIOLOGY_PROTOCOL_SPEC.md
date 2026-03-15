# UART Physiology Protocol Spec

## Status

**Placeholder / draft document** for the UART protocol that will carry live physiological data from an external device into the FPGA.

This file exists so the RF/vector integration work has an explicit place to capture the protocol contract before implementation drifts across RTL, host/device firmware, and docs.

## Intended use

The planned live physiology path is:

```text
external device
  → board UART port
  → FPGA UART ingest logic
  → ECG / EDA sample streams
  → ECG DSP / EDA DSP feature extraction
  → rf_anxiety
```

## Current known constraints

- The existing `rtl/uart_rx.v` receives UART in **8-N-1** format.
- Default baud rate is **115200**.
- Two received bytes are assembled into **one 16-bit little-endian word**.
- A simple two-channel stream carrying ECG and EDA at 700 samples/s each is bandwidth-feasible at 115200 baud.

## Recommended first implementation assumption

Until a more complex framing need is proven, start with:

```text
word 0 = ECG sample
word 1 = EDA sample
word 2 = ECG sample
word 3 = EDA sample
...
```

with each sample represented as a signed 16-bit little-endian word.

## Decisions still required

### 1. Framing model

Choose one:

- raw interleaved sample stream
- fixed-length packets
- framed packets with headers / sync markers

### 2. Sample encoding

Need to define:

- signedness
- ADC scale / units
- allowed numeric range
- saturation / clipping behavior at the sender

### 3. Channel ordering

Need to lock down:

- ECG first, EDA second
- or a packet format with explicit channel tags

### 4. Resynchronization strategy

Need to define behavior after:

- byte loss
- framing error
- sender reset
- FPGA reset mid-stream

### 5. Timing contract

Need to define:

- whether UART words represent instantaneous samples or decimated samples
- whether sender guarantees 700 Hz per channel exactly
- how FPGA derives `sample_valid` for ECG and EDA DSP blocks

### 6. Startup / idle behavior

Need to define:

- what the sender transmits before valid data is available
- whether zeros are legal data or reserved as idle fill
- whether there is a start-of-stream marker

### 7. Debug observability

Need to decide how to inspect:

- received UART words
- ECG/EDA channel split
- sample counters
- framing errors / overruns

## Suggested protocol fields if framing is added later

If raw interleaving proves too fragile, consider a framed packet such as:

```text
SYNC0 SYNC1 TYPE LEN PAYLOAD... CRC
```

Possible payload contents:

- ECG sample
- EDA sample
- timestamp / sequence count
- status flags

## Validation checklist

- [ ] sender and FPGA agree on little-endian word ordering
- [ ] sender and FPGA agree on ECG/EDA channel ordering
- [ ] no sustained UART overrun at the chosen baud rate
- [ ] FPGA can recover cleanly after reset or cable reconnect
- [ ] DSP blocks receive correctly separated ECG and EDA sample streams
- [ ] RF output changes plausibly with controlled physiology stimuli

## Related docs

- `docs/ARCHITECTURE.md`
- `docs/BURN_AND_TEST.md`
- `docs/RF_AND_REAL_CALCULATION_INTEGRATION_PLAN.md`
- `docs/RF_VECTOR_IMPLEMENTATION_HANDOVER.md`
- `docs/debug/USB_DEBUG_STATUS_2026-03-15.md`

## Owner notes

When implementation starts, update this file first with the chosen wire protocol and keep the RTL comments, external sender code, and test procedures aligned with it.
