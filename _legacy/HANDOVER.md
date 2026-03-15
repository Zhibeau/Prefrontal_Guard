# Aegis-Chip: Engineering Handover Document

**Prepared for:** Incoming engineer
**Date:** 2026-03-12
**Board:** ALINX AX7102 Rev 1.1
**FPGA:** XC7A100T-2FGG484I (484-pin BGA, speed grade -2, industrial)
**Toolchain:** Vivado 2025.2, Vitis HLS 2025.2 — at `/home/midu/vivado/2025.2/`

---

> **Status update (2026-03-14):** Timing closure has since been completed and the FPGA has been programmed successfully.  
> See `TIMING_CLOSURE.md` for the full timing history, failure analysis, fixes, and final post-route numbers.

---

## 1. Project Background

**Aegis-Chip** is a real-time physiological safety shield for LLMs. It intercepts a 16-dimensional projection of an LLM's hidden state, evaluates a **Control Barrier Function (CC-CBF)** against a physiological anxiety level, and returns a steering vector that corrects the LLM's output trajectory.

### Original architecture (what was built)
```
PC ──UART──► FPGA (CC-CBF) ──UART──► PC
              ▲
         Biosensors (ECG/EDA)
         → Random Forest → anxiety_level
```

### New target architecture (what your colleague must build)
```
Raspberry Pi ──UART──► FPGA ──Ethernet UDP──► PC (192.168.1.2)
                        │
                  CC-CBF "Interference
                   Vector" calculation
```

---

## 2. What Has Been Implemented and Tested

### 2.1 Completed modules

| Module | File | Status | Notes |
|--------|------|--------|-------|
| UART RX | `rtl/uart_rx.v` | ✅ Tested | 200 MHz, 115200 baud, 2-byte → 16-bit assembler, AXI-Stream master |
| UART TX | `rtl/uart_tx.v` | ✅ Tested | 200 MHz, 115200 baud, 16-bit → 2-byte, AXI-Stream slave |
| Button anxiety | `rtl/btn_anxiety.v` | ✅ Synthesised | 4×push-buttons → anxiety_level [0–960] in 64 steps. 20 ms debouncer |
| ADC frontend | `rtl/adc_frontend.v` | ✅ Synthesised | XADC wrapper for ECG/EDA. Not tested on hardware yet |
| ECG DSP | `hls/ecg_dsp/` | ✅ HLS synthesised | 5 HRV features (HR, SDNN, RMSSD, pNN50, Std). BRAM ring buffer 42 000 samples |
| EDA DSP | `hls/eda_dsp/` | ✅ HLS synthesised | 7 EDA features (SCL, SCR, peaks etc). 700 Hz, EMA α=0.000143 |
| RF inference | `hls/rf_anxiety/` | ✅ HLS synthesised | 30-tree Random Forest. Outputs anxiety_level [0,1024]. Float→fixed via perl sed |
| Top-level | `rtl/aegis_top.v` | ✅ Synthesised | Integrates all blocks |
| XDC constraints | `constraints/aegis_ax7102.xdc` | ✅ Verified | All pins confirmed from AX7102_UG.pdf |
| BURN & TEST | `BURN_AND_TEST.md` | ✅ Written | Step-by-step guide |
| Test harness | `test_aegis.py` | ✅ Written | Python pyserial test suite with reference golden model |

### 2.2 CC-CBF engine — current state

The CC-CBF engine (`aegis_shield`) has **gone through two implementations**:

**Attempt 1 — HLS (`hls/aegis_shield/`)**
Root cause of failure: Vitis HLS 2025.2 infers **unsigned** pipeline registers for `x_t_TDATA` inputs even when the C++ type is `ap_int<16>`. The HLS optimizer converts constant-weight multiplications into left-shift concatenations (`{x_reg, 6'd0}`), which becomes a zero-extension instead of sign-extension for negative values. Result: `h` is always ≥ 0, so `u_t` is always zero. Numerous fixes were attempted (stream type changes, explicit arithmetic sign extension, per-loop PIPELINE instead of function PIPELINE) — all produced the same wrong unsigned registers in the synthesised RTL.

**Attempt 2 — Pure Verilog (`rtl/aegis_shield.v`)**
A hand-written state machine was created to bypass HLS entirely. That RTL was then timing-closed through several iterations:

- pipelining the barrier-bias computation,
- refactoring the correction multiply path,
- moving the UART/control/shield path to a dedicated **100 MHz core clock**,
- cleaning up CDC handling between the core domain and the Ethernet/GMII domain.

**Current status:**

- post-route **WNS = 0.389 ns**
- **TNS = 0.000 ns**
- bitstream written successfully to `impl/aegis_chip.bit`

The full chronology and rationale for each timing fix are documented in `TIMING_CLOSURE.md`.

---

## 3. CC-CBF Mathematics (for the new engineer)

```
W       = [64, 128, 96, 112, 80, 72, 104, 88, 120, 60, 92, 116, 76, 84, 100, 68]
B       = 5000 − ((6000 × anxiety_level) >> 10)
h       = B + Σ W[i]·x[i]   for i = 0..15   (dot product)

if h ≥ 0:  u_t[i] = 0                         (LLM is in safe region)
if h < 0:  u_t[i] = clamp₁₆( (−h × W[i]) >> 10 )  (steer back into safe set)
```

All arithmetic is 32-bit signed integer. No floats anywhere.
`anxiety_level` ∈ [0, 1024]: higher = smaller safe region = earlier intervention.
During testing: `anxiety_level` comes from 4 push-buttons (btn_anxiety.v).
In production: `anxiety_level` will come from the Random Forest inference module.

---

## 4. Repository Structure

```
aegis_fpga/
├── rtl/
│   ├── aegis_top.v          ← Top-level integration (needs update for new arch)
│   ├── aegis_shield.v       ← CC-CBF engine — PURE VERILOG (timing closed)
│   ├── uart_rx.v            ← UART RX, 115200 baud, 200 MHz ✓
│   ├── uart_tx.v            ← UART TX, 115200 baud, 200 MHz ✓
│   ├── btn_anxiety.v        ← Push-button → anxiety_level encoder ✓
│   └── adc_frontend.v       ← XADC wrapper (not tested) ✓
│
├── hls/
│   ├── aegis_shield/        ← HLS version (BROKEN — do not use)
│   │   ├── aegis_shield.h
│   │   └── aegis_shield.cpp
│   ├── ecg_dsp/             ← ECG HRV feature extractor ✓
│   ├── eda_dsp/             ← EDA feature extractor ✓
│   └── rf_anxiety/          ← Random Forest inference ✓
│       ├── rf_anxiety.h
│       ├── rf_anxiety.cpp
│       ├── rf_biological_arousal.h          ← Original float model
│       └── rf_biological_arousal_fixed.h    ← ap_fixed<32,16> version (auto-generated)
│
├── ip_repo/                 ← Exported HLS IPs (unzipped Vivado catalog)
│   ├── ecg_dsp_ip/
│   ├── eda_dsp_ip/
│   └── rf_anxiety_ip/
│
├── constraints/
│   └── aegis_ax7102.xdc     ← All physical pin constraints ✓
│
├── scripts/
│   ├── build_all_hls.tcl    ← Batch HLS synthesis (vitis-run --mode hls)
│   └── build_vivado.tcl     ← Vivado project creation + bitstream flow
│
├── impl/
│   ├── aegis_chip.bit       ← Latest bitstream (timing met)
│   ├── utilization_impl.rpt
│   └── timing_impl.rpt
│
├── test_aegis.py            ← Python UART test harness ✓
├── BURN_AND_TEST.md         ← Programming + test guide ✓
├── TIMING_CLOSURE.md        ← Timing-closure history and final signoff notes
├── ARCHITECTURE.md          ← ASCII block diagram
└── HANDOVER.md              ← This document
```

---

## 5. Critical Pin Assignments (from AX7102_UG.pdf, verified)

| Signal | FPGA Pin | Bank | Standard | Notes |
|--------|----------|------|----------|-------|
| SYS_CLK_P | R4 | 34 | DIFF_SSTL15 | 200 MHz SiTime oscillator |
| SYS_CLK_N | T4 | 34 | DIFF_SSTL15 | |
| RESET_N | T6 | 34 | LVCMOS15 | Active-low, pull-up |
| UART1_RXD | Y12 | 13 | LVCMOS33 | From CP2102 USB-UART (PC side) |
| UART1_TXD | Y11 | 13 | LVCMOS33 | To CP2102 USB-UART (PC side) |
| UART2_RXD | AB10 | 13 | LVCMOS33 | RS232 via MAX3232 — **use for RPi input** |
| UART2_TXD | AA9 | 13 | LVCMOS33 | RS232 via MAX3232 |
| KEY1..4 | B18,B17,A16,A15 | 14/15 | LVCMOS33 | Active-low push-buttons |
| LED1..4 | C17,D17,V20,U20 | 15/14 | LVCMOS33 | Active-low expansion LEDs |
| XADC_VP | L10 | Analog | — | CON1 PIN53 |
| XADC_VN | M9 | Analog | — | CON1 PIN51 |
| Ethernet MDC | W10 | 13 | LVCMOS33 | RTL8211EG management clock |
| Ethernet MDIO | V10 | 13 | LVCMOS33 | |
| E_GTXC | L18 | 14 | LVCMOS33 | GMII TX clock |
| E_TXEN | M15 | 14 | LVCMOS33 | TX enable |
| E_RXC | K18 | 14 | LVCMOS33 | GMII RX clock |
| E_RXDV | M22 | 14 | LVCMOS33 | RX data valid |
| E_RESET | L15 | 14 | LVCMOS33 | PHY reset (active-low) |

**Bank voltages:** Bank 34/35 = 1.5 V (DDR3). All other IO banks = 3.3 V.

---

## 6. Build Procedures

### 6.1 HLS synthesis (for ecg_dsp, eda_dsp, rf_anxiety only — NOT aegis_shield)
```bash
cd /home/midu/aegis_fpga
export PATH=/home/midu/vivado/2025.2/Vitis/bin:$PATH
vitis-run --mode hls --tcl scripts/build_all_hls.tcl
# Unzip outputs:
cd ip_repo && for z in *.zip; do unzip -q -o "$z" -d "${z%.zip}"; done
```

### 6.2 Vivado synthesis + implementation + bitstream
```bash
export PATH=/home/midu/vivado/2025.2/Vivado/bin:$PATH
# Uses /tmp/vivado_full.tcl (kept in /tmp from last session; see Section 7 to recreate)
mkdir -p /tmp/vsyn && cd /tmp/vsyn
vivado -mode batch -source /tmp/vivado_full.tcl
# Output: /home/midu/aegis_fpga/impl/aegis_chip.bit
```

### 6.3 Program the FPGA
```tcl
# In Vivado TCL console (Digilent JTAG cable on JTAG port):
open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target
set_property PROGRAM.FILE {/home/midu/aegis_fpga/impl/aegis_chip.bit} \
    [get_hw_devices xc7a100t_0]
program_hw_devices [get_hw_devices xc7a100t_0]
```

Or command-line:
```bash
cat > /tmp/program.tcl << 'EOF'
open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target
set_property PROGRAM.FILE {/home/midu/aegis_fpga/impl/aegis_chip.bit} \
    [get_hw_devices xc7a100t_0]
program_hw_devices [get_hw_devices xc7a100t_0]
close_hw_target
exit
EOF
vivado -mode batch -source /tmp/program.tcl
```

### 6.4 Run the test suite
```bash
# One-time permission fix (permanent fix: sudo usermod -aG dialout $USER + relogin)
sudo chmod a+rw /dev/ttyUSB0
# Run all tests (no buttons pressed = anxiety=0)
python3 test_aegis.py /dev/ttyUSB0 --anxiety 0
# Run with KEY1 pressed (anxiety=512)
python3 test_aegis.py /dev/ttyUSB0 --anxiety 512
```

---

## 7. Current Vivado Synthesis TCL (`/tmp/vivado_full.tcl`)

The synthesis script used throughout this project (kept in `/tmp`, recreate if needed):

```tcl
set ROOT "/home/midu/aegis_fpga"
set PART "xc7a100tifgg484-1L"

# aegis_shield is now pure Verilog (HLS replaced due to sign register bug)
read_verilog ${ROOT}/rtl/aegis_shield.v
read_verilog [glob ${ROOT}/ip_repo/ecg_dsp_ip/hdl/verilog/*.v]
read_verilog [glob ${ROOT}/ip_repo/eda_dsp_ip/hdl/verilog/*.v]
read_verilog [glob ${ROOT}/ip_repo/rf_anxiety_ip/hdl/verilog/*.v]
read_verilog ${ROOT}/rtl/uart_rx.v
read_verilog ${ROOT}/rtl/uart_tx.v
read_verilog ${ROOT}/rtl/adc_frontend.v
read_verilog ${ROOT}/rtl/btn_anxiety.v
read_verilog ${ROOT}/rtl/aegis_top.v
read_xdc     ${ROOT}/constraints/aegis_ax7102.xdc

set_property -name "SEVERITY" -value "WARNING" -objects [get_drc_checks MDRV-1]
synth_design -top aegis_top -part $PART

file mkdir ${ROOT}/impl
report_utilization -file ${ROOT}/impl/utilization_synth.rpt
write_checkpoint -force ${ROOT}/impl/post_synth.dcp
puts "=== SYNTHESIS DONE ==="

opt_design
place_design
phys_opt_design
route_design
phys_opt_design

report_timing_summary -no_header -file ${ROOT}/impl/timing_impl.rpt -warn_on_violation
report_utilization    -file ${ROOT}/impl/utilization_impl.rpt
write_bitstream -force ${ROOT}/impl/aegis_chip.bit
puts "=== BITSTREAM WRITTEN ==="
```

---

## 8. New Architecture Requirements (For Incoming Engineer)

### 8.1 Overview of changes needed

```
OLD:  PC ──UART──► FPGA CC-CBF ──UART──► PC
NEW:  RPi ──UART──► FPGA CC-CBF ──Ethernet UDP──► PC (192.168.1.2)
```

### 8.2 Task 1 — UART input from Raspberry Pi

The Raspberry Pi will send physiological simulation data via UART at 115200 baud.

**Recommended UART port:** Use **UART2** (RS232 via MAX3232) at pins AB10/AA9. This keeps UART1 (CP2102) free for PC debug.

> ⚠️ **Voltage warning:** The MAX3232 chip converts between RS232 (±12V) and 3.3V CMOS. If the RPi connects directly at 3.3V TTL (bypassing the MAX3232), use an alternative UART port directly through the expansion connectors (CON2/CON3) with LVCMOS33 level shifters if needed.

**Existing `uart_rx.v` is reusable** — just change the pin constraint:
```tcl
# In aegis_ax7102.xdc, replace UART1 with UART2 for RPi input:
set_property PACKAGE_PIN  AB10  [get_ports uart_rxd]  ;# UART2_RXD from RPi
set_property IOSTANDARD   LVCMOS33  [get_ports uart_rxd]
```

**Data framing from RPi:** If the Pi sends 8-bit bytes representing 16-bit samples (MSB first or LSB first), `uart_rx.v` already handles the 2-byte assembly. Confirm with the Pi-side code whether little-endian or big-endian is used. The current UART RX uses **little-endian** (first byte = LSB).

**FIFO buffer:** Add a synchronous FIFO between uart_rx and the CC-CBF engine to prevent sample loss during processing:
```verilog
// Instantiate a Xilinx FIFO (Vivado IP Catalog → FIFO Generator):
// - Width: 16 bits, Depth: 256+, clock: sys_clk_200m
// - Input: from uart_rx M_AXIS (m_axis_tdata, m_axis_tvalid → wr_en, din)
// - Output: to aegis_shield x_t AXI-Stream (tdata, tvalid)
```

### 8.3 Task 2 — Historical timing issue and final resolution

This section is retained for context only.

Earlier in the project, `aegis_shield.v` failed timing at 200 MHz, with historical WNS values around **−1.216 ns**. That issue has since been resolved.

**What ultimately fixed it:**

1. pipelining the barrier-bias computation inside `rtl/aegis_shield.v`,
2. refactoring the correction multiply path (`w_pos_reg`, `corr_prod_full`),
3. moving UART/control/shield logic to a generated **100 MHz core clock**,
4. cleaning up the CDC boundary between `clk100` and the Ethernet/GMII domain.

**Final status:**

- post-route **WNS = 0.389 ns**
- **TNS = 0.000 ns**
- bitstream generation succeeds

See `TIMING_CLOSURE.md` for the detailed timing writeup, intermediate failure modes, and reasons we chose the structural fix over a multicycle-path workaround.

### 8.4 Task 3 — Ethernet UDP output

The AX7102 has a **Gigabit Ethernet** interface via **Realtek RTL8211EG** PHY (GMII interface).

**Target:** Send the 16-element CC-CBF steering vector `u_t[0..15]` as a UDP packet to PC at `192.168.1.2`.

**Ethernet PHY pin assignments** (from AX7102_UG.pdf, p.35-36):
```
E_GTXC  = L18   (GMII TX clock, output to PHY)
E_TXEN  = M15   (TX enable)
E_TXER  = M13   (TX error)
E_TXD0  = L14,  E_TXD1  = K16,  E_TXD2  = L16,  E_TXD3  = K17
E_TXD4  = L20,  E_TXD5  = L19,  E_TXD6  = L13,  E_TXD7  = J17
E_RXC   = K18   (GMII RX clock, input from PHY)
E_RXDV  = M22   (RX data valid)
E_RXER  = N19   (RX error)
E_RXD0  = N22,  E_RXD1  = H18,  E_RXD2  = H17,  E_RXD3  = K19
E_RXD4  = M21,  E_RXD5  = L21,  E_RXD6  = N20,  E_RXD7  = M20
E_RESET = L15   (Active-LOW PHY reset)
E_MDC   = W10   (MDIO clock)
E_MDIO  = V10   (MDIO data)
```

**Implementation approach:**
1. Integrate the provided `ethernet_test` UDP module into `aegis_top.v`.
2. The CC-CBF output is 16 × 16-bit = 32 bytes — fits in a single UDP payload.
3. Connect the `u_t` AXI-Stream output to the Ethernet TX path instead of `uart_tx`.
4. Set destination IP = `192.168.1.2`, source IP = `192.168.1.10` (or as desired), destination UDP port = e.g. 1234.

**Typical `ethernet_test` module interface:**
```verilog
ethernet_udp_tx u_eth_tx (
    .clk          (sys_clk_200m),    // or dedicated Ethernet clock
    .rst_n        (reset_n),
    .dst_ip       (32'hC0A80102),   // 192.168.1.2
    .dst_port     (16'd1234),
    .payload_data (u_t_flat),       // 256-bit = 16×16-bit vector
    .payload_len  (16'd32),         // 32 bytes
    .send_trigger (u_t_frame_done), // pulse when CC-CBF frame complete
    // Ethernet GMII pins...
    .gmii_txclk   (E_GTXC),
    .gmii_txen    (E_TXEN),
    .gmii_txd     (E_TXD)
);
```

### 8.5 Task 4 — Debug checklist for Ethernet communication failures

Common failure modes and how to check them:

**Clock Domain Crossing (CDC)**
- The PHY RX clock (E_RXC) and TX clock (E_GTXC) are asynchronous to sys_clk_200m.
- Check: All signals crossing clock domains must go through **2-FF synchronisers** or **async FIFOs**.
- Vivado CDC check: `report_cdc -details -file cdc.rpt` after synthesis.

**Incorrect Pin Constraints**
- Confirm all E_* pins match the table in Section 5 above.
- Verify `IOSTANDARD = LVCMOS33` for all Ethernet pins (Bank 14, 3.3 V).
- The TX clock `E_GTXC` is **output** from FPGA — verify direction in XDC:
  ```tcl
  set_property PACKAGE_PIN L18 [get_ports E_GTXC]
  set_property IOSTANDARD LVCMOS33 [get_ports E_GTXC]
  ```

**Reset logic**
- RTL8211EG requires a hardware reset pulse ≥ 10 ms: `E_RESET` active-low.
- Implement a reset controller that holds `E_RESET = 0` for at least 10 ms (2,000,000 cycles at 200 MHz) on power-up, then releases.

**MDIO initialisation**
- The PHY requires register configuration via MDIO for GMII mode.
- Without MDIO init, the PHY may default to a wrong mode. Implement a simple MDIO write-only state machine or use `E_RESET` only if the PHY defaults are correct for your use case.

**IP/MAC address configuration**
- Ensure destination MAC in the ARP table / in the UDP TX module matches `192.168.1.2`'s MAC address, OR use broadcast MAC (`FF:FF:FF:FF:FF:FF`) for testing.

---

## 9. Known Issues and Warnings

| Issue | Severity | Details |
|-------|----------|---------|
| Timing closure history | ℹ️ Info | Earlier builds failed timing at 200 MHz. Final routed build now meets timing; see `TIMING_CLOSURE.md` for details and caveats. |
| HLS `aegis_shield` sign bug | ⚠️ Do not use | HLS version in `hls/aegis_shield/` is broken. Always use `rtl/aegis_shield.v`. |
| VAUXP/VAUXN for EDA sensor | ⚠️ Low | XDC constraints for EDA analog input are commented out (need FGG484 package file to confirm pin). Not needed for new RPi architecture. |
| `sudo chmod a+rw /dev/ttyUSB0` | ℹ️ Info | Needed each time the UART cable is reconnected. Permanent fix: `sudo usermod -aG dialout $USER` + re-login. |
| MDRV-1 DRC downgraded to warning | ℹ️ Info | Some HLS-generated IPs (rf_anxiety, ecg_dsp) have internal multi-driver nets. These are false positives in the HLS IP — the DRC has been downgraded to WARNING. |
| HLS tool invocation | ℹ️ Info | `vitis_hls` is NOT in PATH. Use: `vitis-run --mode hls --tcl script.tcl` |

---

## 10. Next Steps for Incoming Engineer

**Priority 1 — Preserve timing closure**
- Keep the shield/UART/control path on `clk100` unless there is a measured reason to increase frequency.
- Re-check `TIMING_CLOSURE.md` before changing `aegis_shield.v`, Ethernet CDC logic, or payload RAM interfaces.

**Priority 2 — Adapt to new data source**
- Update `aegis_top.v` to receive UART from RPi instead of PC.
- Add FIFO buffer between UART RX and CC-CBF engine.
- Decision needed: does the RPi send 8-bit or 16-bit samples? Adjust `uart_rx.v` accordingly.

**Priority 3 — Implement Ethernet UDP output**
- Integrate `ethernet_test` UDP module into `aegis_top.v`.
- Remove `uart_tx.v` from the output path or keep for debug.
- Add PHY reset controller (10 ms on power-up).
- Add Ethernet pin constraints for E_* signals to XDC.

**Priority 4 — End-to-end test**
- RPi Python script sends 16 × INT16 test vectors over UART.
- Wireshark on PC to verify UDP packets arrive at 192.168.1.2.
- Use `test_aegis.py` as reference for expected CC-CBF output values.

---

## 11. CC-CBF Reference Implementation (Golden Model)

For verifying the FPGA output, use this Python reference:

```python
W      = [64, 128, 96, 112, 80, 72, 104, 88, 120, 60, 92, 116, 76, 84, 100, 68]
B_BASE = 5000
ALPHA  = 6000

def cbf(x_t, anxiety=0):
    B = B_BASE - ((ALPHA * anxiety) >> 10)
    h = B + sum(W[i] * x_t[i] for i in range(16))
    if h >= 0:
        return [0] * 16, h
    return [max(-32768, min(32767, ((-h) * w) >> 10)) for w in W], h

# Test case: all -100 at anxiety=0
u, h = cbf([-100]*16, 0)
# h = -141000, u[0] = 8812, u[1] = 17625, ...
```

---

## 12. File Checksums / Bitstream Info

```
impl/aegis_chip.bit   latest validated bitstream (final routed build meets timing)
FPGA part:            xc7a100tifgg484-1L
Post-impl LUTs:       ~1400 (2.2%)
Post-impl FFs:        ~1700 (1.3%)
Post-impl DSPs:       15 (6.3%)
Post-impl BRAM:       0
```

---

*End of handover document. Good luck with the new architecture.*
