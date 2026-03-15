# Aegis-Chip Architecture
## Xilinx Artix-7 XC7A100T-CSG324 @ 200 MHz

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                         AEGIS-CHIP SYSTEM ON FPGA                            ║
║                   XC7A100T-CSG324 · 200 MHz LVDS Clock                       ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                               ║
║  sys_clk_p/n ─── IBUFDS ────────────────── sys_clk_200m ──── (all modules)  ║
║                                                                               ║
╠══════════════╦═══════════════════════════╦═══════════════════════════════════╣
║  PIPELINE A  ║       PIPELINE B          ║         PIPELINE C                ║
║  Physiology  ║      UART Bridge          ║        CC-CBF Engine              ║
╠══════════════╬═══════════════════════════╬═══════════════════════════════════╣
║              ║                           ║                                   ║
║  [Sensors]   ║  PC / LLM Host            ║                                   ║
║  ECG 700 Hz  ║  uart_rxd                 ║                                   ║
║  EDA 700 Hz  ║     │                     ║                                   ║
║     │        ║     ▼                     ║                                   ║
║     ▼        ║  ┌─────────┐  8-bit bytes ║                                   ║
║  ┌────────┐  ║  │ uart_rx │              ║                                   ║
║  │  XADC  │  ║  │  FSM    │              ║                                   ║
║  │ 12-bit │  ║  │(6-state)│              ║                                   ║
║  │ 700 Hz │  ║  └────┬────┘              ║                                   ║
║  └───┬────┘  ║       │ 2-byte assembly   ║                                   ║
║      │       ║       │ (little-endian)   ║                                   ║
║  ┌───▼────┐  ║       │ rx_axis_t*[15:0]  ║                                   ║
║  │adc_    │  ║       ▼                   ║                                   ║
║  │front-  │  ║  ┌──────────────────────────────────────────────┐            ║
║  │end.v   │  ║  │              aegis_shield_0                   │            ║
║  │700 Hz  │  ║  │         (Vitis HLS · ap_ctrl_none)           │            ║
║  │ strobe │  ║  │                                               │            ║
║  └┬──────┘  ║  │  1. B = B_BASE − ((ALPHA × anxiety) >> 10)  │            ║
║   │ecg[15:0]║  │  2. h = W · x_t + B          [UNROLL×16]    │            ║
║   │eda[15:0]║  │  3. if h≥0: u_t = 0                         │            ║
║   │         ║  │     if h<0: u_t = (−h × W) >> 10            │            ║
║  ┌▼──────┐  ║  └──────────────────┬───────────────────────────┘            ║
║  │ecg_   │  ║    tx_axis_t*[15:0] │                                          ║
║  │dsp_0  │  ║                     ▼                                          ║
║  │HLS    │  ║              ┌─────────┐  8-bit bytes                          ║
║  │5 feats│  ║              │ uart_tx │                                        ║
║  └┬──────┘  ║              │  FSM    │                                        ║
║   │         ║              │(4-state)│                                        ║
║  ┌▼──────┐  ║              └────┬────┘                                       ║
║  │eda_   │  ║                   │ uart_txd                                   ║
║  │dsp_0  │  ║                   ▼                                            ║
║  │HLS    │  ║         PC / LLM Host                                          ║
║  │7 feats│  ║                                                                ║
║  └┬──────┘  ║                                                                ║
║   │         ║                                                                ║
║  ┌▼──────┐  ║◄── anxiety_level [15:0] (Q0, range 0–1024) ─────────────────►║
║  │rf_    │  ║                                                                ║
║  │anxiety│  ║                                                                ║
║  │_0 HLS │  ║                                                                ║
║  │30-tree│  ║                                                                ║
║  │RF     │  ║                                                                ║
║  └───────┘  ║                                                                ║
╠══════════════╩═══════════════════════════╩═══════════════════════════════════╣
```

## Module Inventory

| File | Type | Purpose |
|------|------|---------|
| `rtl/aegis_top.v` | Verilog | Top-level integration |
| `rtl/uart_rx.v` | Verilog | 6-state UART RX FSM, 2-byte→16-bit assembler |
| `rtl/uart_tx.v` | Verilog | 4-state UART TX FSM, 16-bit→2-byte serialiser |
| `rtl/adc_frontend.v` | Verilog | XADC wrapper, 700 Hz strobe |
| `hls/ecg_dsp/` | Vitis HLS | R-peak detection, IBI, HR, SDNN, RMSSD, pNN50 |
| `hls/eda_dsp/` | Vitis HLS | EMA SCL/SCR, stats, peak detection |
| `hls/rf_anxiety/` | Vitis HLS | 30-tree RF inference → anxiety_level [0,1024] |
| `hls/aegis_shield/` | Vitis HLS | CC-CBF steering vector computation |
| `constraints/aegis_ax7102.xdc` | XDC | Pin assignments for AX7102 board |
| `scripts/build_all_hls.tcl` | TCL | Vitis HLS batch synthesis (4 IPs) |
| `scripts/build_vivado.tcl` | TCL | Vivado project + bitstream flow |

## Data Flow Summary

```
ECG ADC (700 Hz)  ──► ecg_dsp_0 ──► 5 z-scored features ─┐
EDA ADC (700 Hz)  ──► eda_dsp_0 ──► 7 z-scored features ─┤
                                                           ▼
                                                    rf_anxiety_0 (30-tree RF)
                                                           │
                                                   anxiety_level [0,1024]
                                                           │
PC ──UART──► uart_rx ──► x_t AXI-Stream ──► aegis_shield_0 ◄──┘
                                                    │
                                           u_t AXI-Stream
                                                    │
PC ◄──UART──── uart_tx ◄────────────────────────────┘
```

## Fixed-Point Precision Summary

| Domain | Type | Range | Precision |
|--------|------|-------|-----------|
| UART payload | `ap_int<16>` | −32768..32767 | 1 LSB |
| HLS features | `ap_fixed<32,16>` | −32768..32767 | 1.5×10⁻⁵ |
| anxiety_level bus | `ap_int<16>` | 0..1024 | 1 (integer) |
| CC-CBF accumulator | `ap_int<32>` | −2.1G..2.1G | 1 LSB |
| ECG/EDA accumulators | `ap_int<64>` | Full 64-bit | 1 LSB |

## BRAM Usage Estimate (XC7A100T has 607 KB total)

| Buffer | Size | BRAM |
|--------|------|------|
| ECG signal ring (42 000 × 16-bit) | 84 KB | ~3 RAMB36 |
| EDA signal ring (42 000 × 16-bit) | 84 KB | ~3 RAMB36 |
| EDA SCR ring (42 000 × 16-bit) | 84 KB | ~3 RAMB36 |
| ECG IBI ring (200 × 16-bit) | 0.4 KB | 1 RAMB18 |
| **Total** | **~253 KB** | **~42%** |
