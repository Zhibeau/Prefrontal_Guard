# Documentation Notes

This folder contains repository-organized notes and debugging writeups.

## Contents

- `ARCHITECTURE.md` — current-versus-target architecture note; clarifies that the active checked-in build is USB with a stub `aegis_shield`, while RF and real vector math remain to be re-integrated.
- `BURN_AND_TEST.md` — current USB build/program/test guide, including the verified `--prime-frames 2` workaround and active-build caveats.
- `debug/ETHERNET_DEBUG_STATUS_2026-03-14.md` — Ethernet bring-up status, experiments tried, and next-step recommendations.
- `debug/USB_DEBUG_STATUS_2026-03-15.md` — USB FX2LP bring-up findings, the verified priming workaround, and integration implications for restoring RF + CC-CBF math.
- `RF_AND_REAL_CALCULATION_INTEGRATION_PLAN.md` — staged roadmap for restoring the real steering-vector calculation, integrating RF anxiety inference, and later connecting real ECG/EDA DSP features.
- `RF_VECTOR_IMPLEMENTATION_HANDOVER.md` — practical handover note for the engineer taking over the RF + vector-calculation restoration work.
- `RF_VECTOR_TAKEOVER_PROMPT.md` — copy-paste prompt for the colleague or coding agent taking over the RF + vector-calculation implementation plan.
- `UART_PHYSIOLOGY_PROTOCOL_SPEC.md` — placeholder/specification draft for how an external device will stream ECG and EDA samples into the FPGA over the board UART port.

## Notes

For current USB bring-up and the RF/vector restoration effort, prefer the documents in this `docs/` folder over older root-level notes that may describe the intended or legacy architecture.

Primary project documents remain at the repository root for now, including:
- `HANDOVER.md`
- `TIMING_CLOSURE.md`
- `FPGA_Implementation_Guide.md`
- `BURN_AND_TEST.md`
- `ARCHITECTURE.md`
