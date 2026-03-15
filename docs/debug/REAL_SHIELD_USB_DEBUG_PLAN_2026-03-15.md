# Real Shield USB Debug Plan — 2026-03-15

## Purpose

This note defines the next debugging steps for the current regression where:

- the active USB transport previously communicated with `--prime-frames 2` when `rtl/aegis_shield.v` was the debug stub;
- after replacing the stub path with the real-shield wrapper, host transactions now time out, including primed transactions.

The goal is to determine **exactly where the 16-word request/response contract breaks** without mixing transport, timing, and math changes all at once.

## Current known-good baseline

Before the real-shield swap, the active USB path was:

```text
Host PC
  → FX2 EP2 OUT
  → aegis_usb_bridge
  → aegis_uart_bridge
  → stub aegis_shield
  → aegis_usb_bridge
  → FX2 EP6 IN
  → Host PC
```

With that configuration:

- the first request(s) could still require priming;
- `--prime-frames >= 2` made the measured transaction succeed reliably enough for bring-up;
- the compute block was **not** real CC-CBF math;
- the stub always honored the basic contract:
  - consume 16 input words;
  - produce 16 output words.

## Current regression

The active `rtl/aegis_shield.v` is now a wrapper around the generated HLS core.

Observed effect:

- host writes still appear to complete;
- host reads time out;
- `--prime-frames 2` no longer rescues the transaction.

This suggests the failure is no longer the original FX2 priming quirk alone.

## Working diagnosis

The most likely failure region is:

```text
USB ingress works enough to launch a frame
            ↓
shield-side integration fails to complete a 16-word reply
            ↓
aegis_usb_bridge never finishes S_COLLECT
            ↓
EP6 never receives a complete return payload
            ↓
host bulk read times out
```

Important nuance:

> This does **not** yet prove the arithmetic itself is wrong.

The fault may instead be in:

- shield input handshake;
- shield output handshake;
- HLS wrapper/regslice behavior;
- stream-sideband integration;
- framing mismatch between `aegis_uart_bridge` and the wrapped HLS core.

## Main debug question

For one host request frame of 16 × INT16 values, which of these is happening?

1. the shield consumes **zero** input words;
2. the shield consumes **some but not all** input words;
3. the shield consumes all 16 input words but produces **zero** outputs;
4. the shield produces **some but not all** 16 output words;
5. the shield produces all 16 output words, but `aegis_usb_bridge` still fails to write EP6.

The next steps are designed to answer that question directly.

## Ranked hypotheses

### H1 — Shield output handshake deadlock

Most likely.

Reason:

- the transport used to work with the stub;
- the new failure appeared after swapping the shield implementation;
- `aegis_usb_bridge` only writes EP6 after collecting 16 output words.

### H2 — Shield input handshake mismatch

Possible.

Reason:

- the generated HLS core may not accept the incoming stream under the current wrapper semantics;
- if `x_t_TREADY` never asserts correctly, the bridge chain can stall before computation starts.

### H3 — Generated HLS wrapper/core is stale or incomplete

Possible and already suspicious.

Reason:

- the checked-in HLS-generated Verilog appears surprisingly small for the intended algorithm;
- the local environment could not rebuild the HLS IP from shell due to unavailable HLS tools on `PATH`;
- some generated regslice/ack wiring looked suspicious during inspection.

### H4 — USB bridge back-end regression on the EP6 side

Less likely, but not yet eliminated.

Reason:

- if the shield actually produces all 16 output words, EP6 writeback should be checked next.

## Debug strategy

Use **small proof-oriented experiments**. Do not mix timing, transport, and algorithm edits together.

### Phase 1 — Re-establish the known-good communication baseline

#### Objective

Prove that primed communication still works when the shield contract is trivially satisfied.

#### Action

Temporarily replace the current real-shield wrapper with a minimal responder that:

- drains exactly 16 input words;
- emits exactly 16 output words;
- does not depend on HLS-generated flow-control logic.

This can be either:

- the previous debug stub, or
- an even simpler all-zero 16-word responder.

#### Expected outcomes

- If primed communication returns, the regression is in the new shield integration path.
- If primed communication still fails, re-open the possibility of transport/back-end regression.

### Phase 2 — Instrument the shield boundary

#### Objective

Measure exactly how many words cross the shield interface on each side.

#### Required observability

Add temporary counters or sticky debug registers for:

- input handshakes: `x_t_TVALID && x_t_TREADY`
- output handshakes: `u_t_TVALID && u_t_TREADY`

Track at least:

- whether the count reached 16 on input;
- whether the count reached 16 on output;
- whether output ever started;
- whether output stopped early.

#### Minimal signals to expose

At least one of the following:

- LEDs with coarse states;
- sticky status bits;
- a temporary debug mux onto returned words if practical.

Recommended coarse indicators:

- shield saw at least 1 input word;
- shield saw all 16 input words;
- shield produced at least 1 output word;
- shield produced all 16 output words.

#### Expected interpretation

- input < 16, output = 0 → ingress handshake problem;
- input = 16, output = 0 → shield core never starts or never produces output;
- input = 16, output between 1 and 15 → output deadlock/framing issue;
- input = 16, output = 16 → inspect `aegis_usb_bridge` EP6 write path next.

### Phase 3 — Compare wrapper vs simple RTL core

#### Objective

Distinguish HLS-integration issues from math issues.

#### Action

Implement a simple hand-written RTL core that preserves the exact frame contract:

- consume 16 input words;
- compute/store internal state sequentially;
- emit 16 output words;
- one multiply/accumulate per cycle is acceptable.

This is intentionally slower but easier to reason about.

#### Why this helps

If the simple RTL core works while the HLS wrapper does not, the problem is likely in:

- HLS wrapper integration;
- HLS-generated stream control;
- generated regslice behavior;
- stale exported HLS Verilog.

### Phase 4 — Only then inspect EP6 back-end again

#### Objective

Re-check the USB writeback only if shield-side evidence says 16 outputs were truly produced.

#### Action

If shield output handshakes reach 16 but host still times out:

- inspect `aegis_usb_bridge` state progression after `S_COLLECT`;
- confirm `tx_buf` is being filled;
- confirm EP6 write states run to completion;
- confirm no new backpressure interaction was introduced.

## Proposed experiment order

1. **A/B baseline test**
   - restore trivial responder;
   - rebuild/program;
   - run `test_aegis_usb.py --single "0,...,0" --prime-frames 2`.

2. **Boundary instrumentation**
   - add input/output handshake counters around `aegis_shield`;
   - rebuild/program;
   - observe whether counts reach 16.

3. **Decision based on counts**
   - input not reaching 16 → debug ingress interface;
   - input 16, output 0 → debug shield start/compute path;
   - input 16, output partial → debug output framing/deadlock;
   - input 16, output 16 → debug EP6 writeback.

4. **Replace wrapper with simple RTL real-math core**
   - preserve 16-in/16-out contract;
   - rebuild/program;
   - compare against software golden model.

## Concrete observations to collect

For each build under test, record:

- shield implementation under test;
- whether `--prime-frames 2` was used;
- input handshake count at shield boundary;
- output handshake count at shield boundary;
- whether host write completed;
- whether host read timed out;
- whether any LEDs/sticky flags indicate partial progress.

A simple table format is recommended:

| Build | Shield variant | Prime frames | Input count | Output count | Host result | Notes |
|------|----------------|--------------|-------------|--------------|-------------|------|
| A | old stub | 2 | 16 | 16 | success | baseline |
| B | HLS wrapper | 2 | ? | ? | timeout | current regression |
| C | simple RTL real core | 2 | ? | ? | ? | comparison |

## What not to do during this debug phase

To avoid ambiguity, do **not** do these simultaneously with handshake debugging:

- change FPGA pin mappings;
- change board-level clock source selection;
- change USB endpoint mapping;
- change the host payload format;
- integrate RF anxiety at the same time;
- add UART physiology ingest work.

## Exit criteria for this plan

This debug plan is considered complete when all of the following are true:

1. we know whether the shield boundary saw 16 input words;
2. we know whether the shield boundary produced 16 output words;
3. we can state whether the regression is in:
   - ingress handshake,
   - shield compute/wrapper,
   - output framing,
   - or EP6 writeback;
4. we have one implementation path that restores primed communication;
5. we can resume Phase 1 validation of real `aegis_shield` math against the host golden model.

## Recommended next action

Implement the smallest possible instrumentation around the current `aegis_shield` boundary first.

If instrumentation is too awkward for immediate use on this board revision, fall back to the faster proof:

- restore the trivial 16-in/16-out responder;
- confirm primed communication returns;
- then replace the HLS wrapper with a simple explicit RTL real-math core.
