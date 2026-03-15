# =============================================================================
# build_all_hls.tcl — Synthesise all four Vitis HLS IPs and export them
# =============================================================================
# Usage (from aegis_fpga/ root):
#   vitis_hls -f scripts/build_all_hls.tcl
#
# Each HLS project is synthesised and exported as a Vivado IP Catalog entry
# under ip_repo/<module_name>_ip. The Vivado build script (build_vivado.tcl)
# points to ip_repo/ to find all four IPs.
#
# FPGA target: xc7a100tifgg484-1L  (AX7102 board, confirmed from run_hls.tcl)
# Clock period: 5 ns  →  200 MHz
# =============================================================================

set PART    "xc7a100tifgg484-1L"
set PERIOD  5
set ROOT    [file normalize [file dirname [info script]]/..]
set IPREPO  ${ROOT}/ip_repo

# =============================================================================
proc build_hls_ip {name top_fn src_files part period ip_repo} {
    set proj "${name}_prj"

    open_project  $proj
    set_top       $top_fn

    foreach f $src_files {
        add_files $f
    }

    open_solution "sol1" -flow_target vivado
    set_part      $part
    create_clock  -period $period -name default

    # HLS directives already embedded in source via #pragma
    csynth_design

    # Export as Vivado IP Catalog entry
    export_design -format ip_catalog \
                  -output "${ip_repo}/${name}_ip" \
                  -evaluate verilog

    close_project
    puts "\n✓  ${name} synthesised → ${ip_repo}/${name}_ip\n"
}

# =============================================================================
# 1. ECG DSP — 5 ECG HRV features
# =============================================================================
build_hls_ip \
    ecg_dsp \
    ecg_dsp \
    [list ${ROOT}/hls/ecg_dsp/ecg_dsp.cpp \
          ${ROOT}/hls/ecg_dsp/ecg_dsp.h] \
    $PART $PERIOD $IPREPO

# =============================================================================
# 2. EDA DSP — 7 EDA features
# =============================================================================
build_hls_ip \
    eda_dsp \
    eda_dsp \
    [list ${ROOT}/hls/eda_dsp/eda_dsp.cpp \
          ${ROOT}/hls/eda_dsp/eda_dsp.h] \
    $PART $PERIOD $IPREPO

# =============================================================================
# 3. RF Anxiety — 30-tree Random Forest inference → anxiety_level [0,1024]
# =============================================================================
build_hls_ip \
    rf_anxiety \
    rf_anxiety \
    [list ${ROOT}/hls/rf_anxiety/rf_anxiety.cpp \
          ${ROOT}/hls/rf_anxiety/rf_anxiety.h \
          ${ROOT}/hls/rf_anxiety/rf_biological_arousal_fixed.h] \
    $PART $PERIOD $IPREPO

# =============================================================================
# 4. Aegis Shield — CC-CBF physics engine (steering vector)
# =============================================================================
build_hls_ip \
    aegis_shield \
    aegis_shield \
    [list ${ROOT}/hls/aegis_shield/aegis_shield.cpp \
          ${ROOT}/hls/aegis_shield/aegis_shield.h] \
    $PART $PERIOD $IPREPO

puts "=== All HLS IPs synthesised successfully. ==="
puts "=== IP Catalog entries in: ${IPREPO} ==="
puts "=== Now run: vivado -mode batch -source scripts/build_vivado.tcl ==="

exit
