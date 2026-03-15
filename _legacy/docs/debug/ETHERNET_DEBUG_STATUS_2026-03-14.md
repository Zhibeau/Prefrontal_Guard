# Ethernet Bring-Up Status — 2026-03-14

## Scope

This note summarizes the current status of AX7102 Ethernet bring-up on the `RTL8211EG` GMII path, including the vendor demo investigation, the experiments already run, observed hardware behavior, and the current best next steps.

> Note: no external human FPGA engineer was consulted directly. A fresh internal second-pass design review was performed to simulate a senior engineering review and is summarized below.

## Current headline

The FPGA is **generating transmit activity**, the PHY reports **link up at 1 Gbps**, and the GMII TX path appears active at the FPGA pins, but the Raspberry Pi still sees **zero packets**.

This strongly suggests the failure is now **below the packet generator** and likely in one of these areas:
- PHY-side mode / initialization assumptions
- GMII TX clock/data timing at the PHY interface
- output pad electrical constraints / signal integrity
- board-specific RTL8211EG configuration that the vendor demo never fully handled

## Confirmed observations

### Pi-side behavior
- Link comes up reliably
- Pi reports `1 Gbps full duplex`
- `RX packets = 0`
- `rx_crc = 0`
- `tcpdump` sees nothing

### LED diagnostics
Latest debug image behavior:
- `LED1 on`
- `LED2 on`
- `LED3 blinking`
- `LED4 blinking`
- LEDs 3 and 4 blink at the same pace

Meaning of the debug LEDs in the instrumented vendor demo:
- `LED1`: reset released / core running
- `LED2`: PHY link detected
- `LED3`: internal MAC TX activity
- `LED4`: physical GMII TX output activity (`e_txen` side)

### Interpretation of those LEDs
This is a major narrowing result:
- `mac_test` is generating frames
- traffic survives through the FPGA-side TX path
- GMII-side physical TX activity is present at the top level
- the failure is **not** simply “the MAC never sends”

## What has already been tried

### 1. Rebuilt and flashed vendor demo bitstream
Used the active project top actually compiled by Vivado:
- `ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/imports/src/ethernet_test.v`

### 2. Fixed undriven `e_txer`
Patched the active vendor demo top to drive:
- `assign e_txer = 1'b0;`

Result:
- no packets on Pi

### 3. Added PHY reset pulse
Patched the active vendor demo top to generate a power-up reset pulse and delay logic release.

Result:
- no packets on Pi

### 4. Added TX-path LED instrumentation
Instrumented the vendor demo top so LEDs indicate:
- reset released
- link detected
- internal TX activity
- physical GMII TX activity

Result:
- LEDs show TX activity both internally and at GMII output side
- still no packets on Pi

### 5. Changed GMII TX clock forwarding
Replaced the fragile direct forwarding approach with:
- buffered internal TX clock
- forwarded `e_gtxc` via `ODDR`

Result:
- build clean
- still no packets on Pi
- LED behavior unchanged

### 6. Upgraded MDIO initialization
Replaced the weak `smi_config` behavior with a fuller Clause-22 sequence modeled on the dormant MIIM controller already in the repo:
- write reg `4 = 0x0141` (10/100 FD advertisement)
- write reg `9 = 0x0200` (1000 FD advertisement)
- write reg `0 = 0x9140` (soft reset + autoneg enable)
- poll regs `1`, `5`, `10` to derive link/speed

Result:
- image builds and programs cleanly
- Pi retest still pending at the time this summary was written

## Important files involved

### Active vendor demo top
- `ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/imports/src/ethernet_test.v`
- this is the top actually referenced by `eth_test.xpr`

### Active MDIO config logic
- `ethernet_test/rgmii_ethernet/src/mdio/smi_config.v`

### Packet generator / MAC test logic
- `ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/mac_test.v`

### GMII arbitration logic
- `ethernet_test/rgmii_ethernet/src/arbi/gmii_arbi.v`

### Vendor project
- `ethernet_test/rgmii_ethernet/eth_test.xpr`

## Build/programming status

Recent vendor-demo debug variants all built and programmed successfully.

Representative successful post-route numbers from the later experiments:
- GTXC-forwarding experiment: `WNS = 1.013 ns`, `TNS = 0.000`
- fuller MDIO-init experiment: `WNS = 1.081 ns`, `TNS = 0.000`

Programming consistently completes with:
- `End of startup status: HIGH`

## Internal senior-style review summary

A fresh internal review ranked the strongest remaining hypotheses as follows.

### Highest-priority hypothesis: TX output pad electrical constraints are missing
The review flagged that the main constraints being used do **not** explicitly set output drive / slew for:
- `e_gtxc`
- `e_txen`
- `e_txd[*]`

This matters because at `1 Gbps GMII (125 MHz)` it is entirely plausible to have:
- link up
- internal TX activity
- but no decodable packets at the receiver

if output edges are too slow or marginal for PHY sampling.

A strong clue is that the alternate diagnostic XDC branch had explicitly added `SLEW FAST` and `DRIVE 12` style constraints for TX outputs.

### Second-priority hypothesis: board-specific PHY mode assumption
Even after better reset and MDIO setup, the PHY may still require:
- a board-specific mode strap assumption
- vendor-specific MDIO page/register setup
- or a different GMII/MII transmit clock contract than the demo assumes

### Third-priority hypothesis: GMII timing/alignment still wrong at the I/O boundary
Even though GTXC forwarding via `ODDR` was tried, more evidence may be needed to prove:
- TX clock phase at the PHY pin is correct
- TX data is aligned with that clock at the physical output cells

## Recommended next experiments

### Experiment 1 — add explicit TX pad electrical constraints
Apply output constraints on the active TX GMII ports in the active XDC, specifically for:
- `e_gtxc`
- `e_txen`
- `e_txd[*]`

Suggested direction from the review:
- `SLEW FAST`
- `DRIVE 12`

This is the fastest high-value test and may resolve a pure signal-integrity / edge-rate issue.

### Experiment 2 — add ILA on actual GMII TX waveforms
Probe at least:
- internal MAC TX data / enable
- post-arbiter GMII TX data / enable
- `e_gtxc`
- link/speed status

Goal:
- capture whether a real Ethernet frame with valid preamble and structure is present at the physical TX boundary

### Experiment 3 — verify or force PHY interface assumptions
If pad constraints do not fix it, the next likely branch is:
- board-specific PHY mode programming
- verifying whether the AX7102 path really behaves as the vendor demo assumes under current RTL8211EG setup

## Red flags noted during review

- The repo contains multiple partially overlapping Ethernet examples, including both RGMII-style and GMII-style designs. It is easy to reason from the wrong source file if not careful.
- The dormant MIIM stack and the active `smi_config.v` had diverged; the active code had been much weaker until upgraded.
- Earlier stale notes in the repo and handover material are not always aligned with the actual `eth_test.xpr` source set.

## Bottom line

Current best conclusion:

1. The transmit path inside the FPGA is very likely alive.
2. The issue is now centered on the **PHY interface boundary**.
3. The most practical next move is **explicit TX output drive/slew constraints**, followed by **ILA capture** if needed.

## Suggested immediate next action

If work resumes from this point, the recommended order is:
1. add TX output `SLEW/DRIVE` constraints on active GMII TX outputs
2. rebuild and retest on the Pi
3. if still failing, instrument with ILA and capture one attempted frame

---

Prepared on: `2026-03-14`
Workspace: `/home/midu/aegis_fpga`
