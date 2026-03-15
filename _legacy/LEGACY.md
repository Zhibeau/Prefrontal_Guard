# Legacy Archive

**Date archived:** 2026-03-14
**Active build:** USB CC-CBF (CY68013A FX2LP parallel FIFO, `aegis_top_usb.v`, bitstream `impl/aegis_chip_usb.bit`)

Everything in this directory is superseded by the USB transport build.
Files are preserved for reference only — do not use in new builds.

---

## RTL (`rtl/`)

| File | Was for | Why legacy |
|------|---------|------------|
| `aegis_top.v` | Original top-level, UART-only transport | Replaced by `aegis_top_usb.v` (FX2LP parallel FIFO) |
| `adc_frontend.v` | XADC wrapper, standalone | Integrated/superseded; USB build uses different signal path |
| `aegis_vector_to_ram.v` | Vector-to-BRAM helper for UART build | Not needed in USB build |
| `eth_smoke_top.v` | Ethernet smoke-test top (RGMII) | Ethernet transport abandoned; USB is final |
| `eth_smoke_alt_top.v` | Alternate Ethernet smoke-test top | Same reason |

---

## Constraints (`constraints/`)

| File | Was for | Why legacy |
|------|---------|------------|
| `aegis_ax7102_eth_alt.xdc` | Ethernet alt pin assignments (RGMII, MII) | Ethernet transport abandoned |

---

## Scripts (`scripts/`)

| File | Was for | Why legacy |
|------|---------|------------|
| `build_vivado.tcl` | Vivado build for UART-only design | Replaced by `build_vivado_usb.tcl` |
| `build_eth_smoke.tcl` | Ethernet smoke-test Vivado build | Ethernet abandoned |
| `build_eth_smoke_alt.tcl` | Alternate Ethernet smoke-test build | Ethernet abandoned |
| `build_vendor_demo.tcl` | ALINX vendor demo build (GTX, MDIO) | Was for bringup only, never production |
| `program_demo_fpga.tcl` | Program vendor demo bitstream | Obsolete |
| `program_fpga.tcl` | Program original UART bitstream | Replaced by `program_fpga_usb.tcl` |
| `run_hls_legacy.tcl` | Early HLS run script, wrong part (xc7a100tcsg324-1) | Replaced by `build_all_hls.tcl` with correct part |

---

## Host Software (`host/`, `run_upload.py`, `upload.sh`, `test_aegis.py`)

| File/Dir | Was for | Why legacy |
|----------|---------|------------|
| `host/` | UDP listener for Ethernet transport | Ethernet abandoned |
| `run_upload.py` | Upload via UART/UDP | Replaced by `test_aegis_usb.py` (FX2LP USB) |
| `upload.sh` | Shell wrapper for upload | Replaced by USB workflow |
| `test_aegis.py` | Integration test for UART build | Replaced by `test_aegis_usb.py` |

---

## Vivado Projects (`vivado_proj*`, `ethernet_*`, `usb_test/`)

| Dir | Was for | Why legacy |
|-----|---------|------------|
| `vivado_proj/` | Main UART-build Vivado project | Superseded by `vivado_proj_usb/` |
| `vivado_proj_eth_smoke/` | Ethernet smoke-test Vivado project | Ethernet abandoned |
| `vivado_proj_eth_smoke_alt/` | Alternate Ethernet Vivado project | Ethernet abandoned |
| `ethernet_smoke_alt_prj/` | Standalone Ethernet alt experiment | Ethernet abandoned |
| `ethernet_test/` | RGMII Ethernet bringup experiments | Ethernet abandoned |
| `usb_test/` | Early USB (not FX2LP) test | Superseded by FX2LP design in `vivado_proj_usb/` |

---

## Implementation Artifacts (`impl/`)

| File | Was for | Why legacy |
|------|---------|------------|
| `aegis_chip.bit` | Bitstream from UART build | Replaced by `impl/aegis_chip_usb.bit` |
| `post_synth.dcp` | Post-synthesis checkpoint, UART build | Stale; USB build has its own checkpoints in `vivado_proj_usb/` |
| `power.rpt`, `power_impl.rpt` | Power reports, UART build | Superseded by USB build reports |
| `timing_impl.rpt`, `timing_synth.rpt`, `timing_summary.rpt` | Timing reports, UART build | Superseded by `impl/timing_summary_usb.rpt` |
| `timing_eth_smoke.rpt` | Timing from Ethernet smoke test | Ethernet abandoned |
| `utilization.rpt`, `utilization_impl.rpt`, `utilization_synth.rpt` | Utilization reports, UART build | Superseded by `impl/utilization_usb.rpt` |
| `utilization_eth_smoke.rpt` | Utilization from Ethernet smoke test | Ethernet abandoned |

---

## IP Zip Archives (`ip_repo/*.zip`)

All zip archives are old export snapshots of HLS IPs from intermediate build iterations.
The live, unzipped IP directories (`aegis_shield_ip/`, `ecg_dsp_ip/`, `eda_dsp_ip/`, `rf_anxiety_ip/`)
remain in `ip_repo/` and are used by the active USB build.

| File | Notes |
|------|-------|
| `aegis_shield_ip.zip` – `aegis_shield_ip7.zip` | Seven successive export snapshots of aegis_shield HLS IP during timing closure |
| `aegis_shield_echo.zip`, `aegis_shield_echo2.zip` | Echo/loopback test versions of aegis_shield |
| `ecg_dsp_ip.zip`, `eda_dsp_ip.zip`, `rf_anxiety_ip.zip` | Old zip snapshots; directories are the authoritative source |

---

## Logs (`logs/`)

Build and programming logs from Ethernet bringup, vendor demo debugging, and HLS runs.
Kept for diagnostic reference.

| File(s) | From |
|---------|------|
| `build_eth_smoke_alt.log` | Ethernet smoke-test alt build |
| `build_vendor_demo*.log` | Multiple vendor demo build attempts (GTX, MDIO, pad-fix variants) |
| `hls_run_tcl*.log` | HLS build runs (old script, old part) |
| `program_demo_fpga*.log` | Vendor demo programming attempts |
| `vivado/` | Vivado session logs |

---

## ML / Old HLS Projects

| File/Dir | Was for | Why legacy |
|----------|---------|------------|
| `aegis_fpga_ml_inference.cpp` | Early standalone ML inference sketch | Replaced by proper HLS modules in `hls/` |
| `rf_biological_arousal.h` | Random forest header without ap_fixed casts | Replaced by `hls/rf_anxiety/rf_biological_arousal_fixed.h` |
| `aegis_ml_prj/` | First HLS project for ML inference exploration | Superseded by `hls/rf_anxiety/` |
| `aegis_shield_prj/` | Intermediate HLS project for aegis_shield | Superseded by `hls/aegis_shield/` |

---

## Old Docs / Handovers

| File | Notes |
|------|-------|
| `Biodata_Stress_ML_Report.md` | Early ML feasibility report; design has evolved significantly |
| `FPGA_Implementation_Guide.md` | Implementation guide for UART build; superseded by `ARCHITECTURE.md` + `BURN_AND_TEST.md` |
| `HANDOVER.md` | First handover document (UART build) |
| `HANDOVER_USB.md` | Intermediate USB handover; superseded by current `BURN_AND_TEST.md` |
| `TIMING_CLOSURE.md` | Notes from timing closure effort on UART build; resolved, USB build meets timing |
| `docs/debug/` | Debug notes from Ethernet/UART bringup sessions |

---

## Vivado Junk (`vivado_junk/`)

Leftover Vivado journal and log files from interactive sessions.
No build-reproducibility value; archived to keep the repo root clean.
