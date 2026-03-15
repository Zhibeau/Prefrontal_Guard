# Aegis-Chip Timing Closure Notes

**Audience:** FPGA / RTL colleagues picking up the design  
**Board:** ALINX AX7102 Rev 1.1  
**FPGA:** XC7A100T-1L / XC7A100T family target in Vivado flow  
**Toolchain:** Vivado 2025.2  
**Last validated:** 2026-03-14

---

## Why this document exists

This project went through several rounds of timing work while the design was being migrated from a simple UART loopback architecture to a mixed-clock **UART-in → CC-CBF compute → UDP/Ethernet-out** system.

The short version is:

- the original pure-Verilog `aegis_shield` implementation was functionally correct but too aggressive at **200 MHz**,
- the first fixes improved slack but did **not** close timing,
- the real breakthrough was to stop forcing the entire control/compute path to run at 200 MHz,
- after that, the remaining failures were mostly **clock-domain crossing / Ethernet-domain integration** issues,
- the final routed build meets timing with:
  - **WNS = 0.389 ns**
  - **TNS = 0.000 ns**
  - `impl/aegis_chip.bit` written successfully.

If you only need the final state, jump to [Final timing status](#final-timing-status). If you want the full “what broke, why, and how we fixed it” history, keep reading — it earned its own postmortem.

---

## System timing context

The design no longer behaves like a single-clock toy datapath. The relevant clock domains in the final implementation are:

| Clock | Frequency | Role |
|---|---:|---|
| `clk200` | 200 MHz | Board input clock domain from LVDS oscillator |
| `clk100` | 100 MHz | Core/control domain for UART, `aegis_shield`, and payload packing |
| `eth_rx_clk` | 125 MHz | Ethernet/GMII domain driven from PHY RX clock |

### Why this matters

The timing problems changed as the architecture changed:

1. At first, the issue was a **plain intra-domain critical path** inside the 200 MHz shield/control logic.
2. After pipelining, the next bottleneck became another **internal multiply/carry path** in the correction logic.
3. After moving the shield/control path to 100 MHz, the remaining failures moved into **cross-domain integration around Ethernet and PHY management**.

That progression is important: timing did not fail for one single reason. It failed for different reasons at different stages.

---

## Baseline failing state

Before the closure work, the key symptoms were:

- **200 MHz global core clock** for UART, shield, and surrounding control logic,
- functionally correct pure-Verilog `rtl/aegis_shield.v`,
- implementation timing failure around **WNS ≈ -1.216 ns**,
- worst path dominated by arithmetic associated with the **barrier bias / anxiety scaling path**.

From the historical implementation report:

- **WNS:** `-1.216 ns`
- **TNS:** `-33.390 ns`
- **Failing endpoints:** `55`
- worst path: `u_btn_anxiety/...stable_reg` → `u_aegis_shield/B_reg_reg[31]/D`

### What that meant in practice

The shield datapath was doing too much in one 5 ns cycle:

- button/anxiety-derived value fanout,
- arithmetic to compute the barrier bias $B$,
- DSP work,
- carry-chain propagation,
- final register capture.

Vivado was basically telling us: “Yes, the math is fine. No, physics is not impressed.”

---

## Chronology of the fixes

## Phase 1 — Pipeline the barrier-bias computation

### Problem

The earliest worst path was not the dot-product receive path anymore; it had shifted to the logic that computes the dynamic barrier threshold:

$$
B = 5000 - \left(\frac{6000 \times anxiety\_level}{2^{10}}\right)
$$

In RTL terms, that path involved:

- sampling/sanitizing the incoming anxiety level,
- multiplying by the scaling constant,
- subtracting from the base threshold,
- feeding the result into `B_reg`.

At 200 MHz, that was too much to ask in one cycle.

### Fix

`rtl/aegis_shield.v` was restructured to insert explicit pipeline states for barrier preparation:

- `S_PREP_MUL`
- `S_PREP_B`

Supporting registers were added so the expensive arithmetic no longer had to complete in one cycle.

### Why it helped

This split the logical work into cleaner sequential stages:

1. capture the anxiety snapshot,
2. compute the scaled term,
3. compute/register `B`.

That reduced the critical combinational depth seen by any single register-to-register path.

### Result

Slack improved materially, but timing still failed:

- WNS improved from roughly **-1.216 ns** to around **-0.96 ns**.

### Lesson

Pipelining worked, but it only exposed the **next real bottleneck**.

---

## Phase 2 — Refactor the correction multiply path

### Problem

After Phase 1, the worst path migrated inside the **correction-vector** logic, i.e. the path that computes the corrective steering term when $h < 0$.

Conceptually, that logic computes something like:

$$
 u_t[i] = \mathrm{clamp}_{16}\left(\frac{(-h) \cdot W[i]}{2^{10}}\right)
$$

The implementation still had avoidable pressure from:

- direct dependence on the weight slice used late in the cycle,
- insufficiently comfortable intermediate width,
- multiplication result feeding a carry-heavy path.

### Fix

The correction datapath in `rtl/aegis_shield.v` was refactored to:

- register a dedicated **positive weight** copy: `w_pos_reg`,
- store the multiply result in a wider register: `corr_prod_full`.

### Why it helped

This did two things:

1. reduced late-arriving logic on the multiplier inputs,
2. removed width pressure / awkward downstream reconstruction around the multiply result.

In other words, we stopped asking the DSP and the carry network to solve both timing and bookkeeping at the same time.

### Result

Timing improved again, but still did not fully close:

- WNS improved to roughly **-0.77 ns**.

### Lesson

At this point the implementation had become a strong hint that the real question was architectural, not just local RTL polish:

> Do we actually need the shield/control path to run at 200 MHz?

The answer was “not even remotely.”

---

## Phase 3 — Move the core logic to 100 MHz

### Problem

The UART and shield path simply did not need a 5 ns cycle time.

Relevant reality check:

- UART input is only **115200 baud**,
- the shield operates on a tiny **16-word frame**,
- the system has a very large cycle budget relative to incoming data rate.

Keeping the entire control/compute path at 200 MHz was creating timing pain without buying meaningful throughput.

### Fix

A divided core clock was introduced in `rtl/aegis_top.v`:

- `sys_clk_div2` flip-flop divides the 200 MHz board clock by 2,
- a `BUFG` creates the routed `clk100` domain,
- UART/control/`aegis_shield`/vector packing logic were moved onto `clk100`.

A generated-clock constraint was added in `constraints/aegis_ax7102.xdc` so Vivado understood the relationship correctly.

### Why it helped

This was the turning point.

Instead of trying to squeeze the shield arithmetic into **5 ns**, the design now had **10 ns** for the core compute/control path. That matched the actual throughput needs and immediately relaxed the hardest arithmetic paths.

### Result

The internal shield/control domain became healthy:

- final `clk100` WNS in the routed design: **2.947 ns**.

That is a comfortable margin, not a last-second scrape.

### Lesson

This was the most important timing decision of the project:

> We closed timing by aligning clock frequency with actual data-rate requirements, not by heroically forcing every block to run “fast because FPGA.”

---

## Phase 4 — Clean up the cross-domain Ethernet integration

### Problem

After the 100 MHz move, the next failures were no longer inside the shield math. They showed up around signals crossing between:

- `clk100` core/control domain, and
- `eth_rx_clk` GMII/Ethernet domain.

The main trouble spots were:

- PHY-management readiness/control signals,
- reset-release sequencing,
- Ethernet-side modules consuming control coming from the slower core domain.

At this stage, the timing story changed from “big arithmetic path” to “CDC discipline.”

### Fixes

#### 1. Move PHY-management work into the GMII domain

`smi_config` was moved so that its operational clocking aligned with the Ethernet/GMII side instead of being driven from the core domain.

This reduced unnecessary cross-domain interactions at the point where PHY setup and Ethernet control meet.

#### 2. Synchronize `phy_ready` into the Ethernet domain

A classic two-flop synchronizer was added:

- `phy_ready_meta`
- `phy_ready_gmii`

These registers synchronize the readiness indication into `eth_rx_clk` before Ethernet-side logic consumes it.

#### 3. Mark synchronizer registers appropriately

The synchronizer registers were marked with `ASYNC_REG` so implementation tools treat them as CDC synchronizer stages rather than ordinary datapath registers.

#### 4. Constrain the first synchronization stage appropriately

A false path was added for the **first-stage capture** into the synchronizer in `constraints/aegis_ax7102.xdc`.

This matters because the first synchronizer stage is intentionally sampling an asynchronous signal; treating that stage like a normal synchronous datapath can produce misleading “failures.”

### Why it helped

These changes ensured that:

- the Ethernet domain saw locally synchronized control inputs,
- Vivado did not waste effort trying to time inherently asynchronous first-stage synchronizer behavior as if it were normal logic,
- the remaining worst paths reflected real synchronous work rather than CDC bookkeeping noise.

### Result

After CDC cleanup and final route/physopt:

- overall timing closed,
- bitstream generation succeeded,
- the final worst path moved to a real Ethernet-side datapath with positive slack.

---

## Final timing status

From `impl/timing_summary.rpt` (post-route, 2026-03-14):

- **WNS:** `0.389 ns`
- **TNS:** `0.000 ns`
- **Hold WNS:** `0.028 ns` on `clk100`
- **All user specified timing constraints are met**

### Clock-domain summary

| Domain | WNS | Notes |
|---|---:|---|
| `clk100` | `2.947 ns` | Core shield/UART/payload-pack logic is now comfortable |
| `eth_rx_clk` | `0.389 ns` | Final global worst setup slack lives here |
| `clk100 -> clk200` | `3.026 ns` | Generated-clock relationship is healthy |

### Final worst setup path

The final worst setup path is in the Ethernet domain:

- source: UDP payload RAM read path,
- destination: `u_udp/ipsend_inst/datain_reg_reg[5]`,
- clock: `eth_rx_clk` at 125 MHz,
- slack: **0.389 ns**.

This is acceptable for the current build, but it is also the path to keep an eye on if the UDP/Ethernet logic grows.

### Final worst hold paths

Hold is clean, but not luxurious everywhere:

- `clk100` worst hold slack: **0.028 ns**,
- `eth_rx_clk` worst hold slack: **0.055 ns**.

That means the design is valid, but colleagues should avoid casual floorplanning or “small harmless” rewrites around RAM/Ethernet datapaths without rechecking hold.

---

## What specifically changed in the RTL / constraints

### `rtl/aegis_shield.v`

Main timing-oriented changes:

- added pipeline states for barrier preparation:
  - `S_PREP_MUL`
  - `S_PREP_B`
- registered anxiety-related intermediate values,
- split late arithmetic into smaller sequential chunks,
- introduced `w_pos_reg` for the correction path,
- widened correction-product storage to `corr_prod_full`.

### `rtl/aegis_top.v`

Main timing-oriented changes:

- created divide-by-2 core clock from the 200 MHz board clock,
- buffered it into a dedicated `clk100` domain,
- moved UART/control/shield/vector-to-RAM logic onto `clk100`,
- kept Ethernet-side work tied to the GMII/PHY-related domain.

### `constraints/aegis_ax7102.xdc`

Main timing-oriented changes:

- added generated-clock definition for `clk100`,
- added CDC guidance for the `phy_ready` synchronizer,
- false-pathed the first synchronizer capture stage.

---

## Why we did **not** use a multicycle-path band-aid

A multicycle path had been considered earlier as a quick workaround. We intentionally did **not** rely on that as the primary solution.

### Reason

The failing logic was not a “naturally multicycle” datapath hidden behind stable enables; it was a design-frequency mismatch and later a CDC-structure problem.

Using a multicycle exception there would have been risky because it could:

- hide genuinely over-constrained logic that should be pipelined or re-clocked,
- make maintenance harder for the next engineer,
- blur the line between valid exceptions and wishful thinking.

The chosen fixes were structural and easier to justify:

- pipeline where the math was too deep,
- slow the non-throughput-critical domain to 100 MHz,
- synchronize CDC boundaries correctly.

That is much easier to defend in code review and much safer to extend.

---

## Remaining caveats / things that are still a bit ugly

Timing is closed, but the design is not yet “cleanroom perfect.”

### 1. Methodology warnings still exist

The final report still shows methodology/check-timing warnings such as:

- missing input/output delays,
- async-driver related warnings,
- a latch-related warning,
- one unconstrained internal endpoint,
- `smi_config` state bits reported in a constant-clock style condition.

These warnings do **not** prevent the current build from meeting the user-specified constraints, but they should be cleaned up before calling the design production-grade.

### 2. Ethernet domain is now the limiting domain

The final global WNS is no longer in `aegis_shield`; it is in the Ethernet-side payload path.

That is good news — the compute path is fixed — but it also means future Ethernet feature growth should be done carefully.

### 3. Hold margins are positive but thin in a few places

The build is legal, but the smallest positive hold slacks are not enormous. Re-run implementation and inspect hold after any of the following:

- changing BRAM packing in `u_vector_to_ram` or payload RAM,
- modifying UDP payload read timing,
- restructuring GMII buffer logic,
- adding debug taps or ILAs near the current critical regions.

---

## Practical guidance for future edits

If timing regresses, check these in order:

1. **Did someone move shield/control logic back to 200 MHz?**  
   If yes, that is suspect number one.

2. **Did `aegis_shield.v` lose one of its pipeline stages?**  
   Especially around `B` computation or correction multiply.

3. **Did a CDC signal start crossing directly from `clk100` to `eth_rx_clk` again?**  
   If yes, fix the CDC instead of trying to constrain around it blindly.

4. **Did the Ethernet payload path grow wider or gain extra fanout?**  
   That is where the final worst path now lives.

5. **Did someone add timing exceptions before understanding the path?**  
   Delete the questionable exception and inspect the real path first.

### Good habits that helped

- reading the actual post-route worst path instead of guessing,
- fixing one bottleneck at a time,
- re-running implementation after each structural change,
- preferring architecture fixes over heroic exception files,
- treating CDC warnings as design work, not report noise.

---

## Reproducing the final timing result

From the repo root:

1. run the normal Vivado build flow via `scripts/build_vivado.tcl`,
2. inspect:
   - `impl/timing_summary.rpt`
   - `impl/timing_impl.rpt`
   - `impl/post_synth.dcp`
   - `impl/aegis_chip.bit`

The validated post-route report for the successful build is the one timestamped **2026-03-14 11:41:20** in `impl/timing_summary.rpt`.

---

## Executive summary for handoff

If a colleague asks “what actually fixed timing?”, the honest answer is:

1. **Pipeline the shield math** where the barrier-threshold and correction paths were too deep.
2. **Stop running the shield/UART/control path at 200 MHz** when 100 MHz is more than enough.
3. **Treat Ethernet integration as a CDC problem**, not just a routing problem.
4. **Constrain synchronizers correctly** so the report reflects real synchronous paths.

That combination turned the project from:

- **failing at about -1.216 ns WNS**

into:

- **passing at +0.389 ns WNS**.

Not magic. Just several rounds of refusing to let the critical path gaslight us.
