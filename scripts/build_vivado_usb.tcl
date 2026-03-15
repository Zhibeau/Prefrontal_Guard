# =============================================================================
# build_vivado_usb.tcl — Vivado build for Aegis-Chip USB transport
# =============================================================================
# Usage (from aegis_fpga/ root, AFTER running build_all_hls.tcl):
#   vivado -mode batch -source scripts/build_vivado_usb.tcl
#
# Top module : aegis_top_usb
# Transport  : CY68013A EZ-USB FX2LP (16-bit parallel slave FIFO)
# Constraints: constraints/aegis_ax7102_usb.xdc
#
# Output: impl/aegis_chip_usb.bit
# =============================================================================

set PART    "xc7a100tifgg484-1L"
set ROOT    [file normalize [file dirname [info script]]/.. ]
set PROJDIR ${ROOT}/vivado_proj_usb

# =============================================================================
# 1. Create project
# =============================================================================
file mkdir ${PROJDIR}
create_project aegis_chip_usb ${PROJDIR} -part ${PART} -force

set_property target_language  Verilog [current_project]
set_property simulator_language Mixed  [current_project]

# =============================================================================
# 2. Add RTL sources
#    All files in rtl/ are included; Vivado will ignore those not reachable
#    from aegis_top_usb (uart_rx.v, aegis_vector_to_ram.v, eth_smoke*.v etc.)
# =============================================================================
add_files -norecurse [glob ${ROOT}/rtl/*.v]

set_property top aegis_top_usb [current_fileset]

# =============================================================================
# 3. Add XDC constraints (USB-specific, no Ethernet)
# =============================================================================
add_files -fileset constrs_1 -norecurse \
    ${ROOT}/constraints/aegis_ax7102_usb.xdc

# =============================================================================
# 4. Synthesis
# =============================================================================
puts "=== Starting Synthesis (USB build) ==="
launch_runs synth_1 -jobs 4
wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    error "Synthesis FAILED. Check ${PROJDIR}/aegis_chip_usb.runs/synth_1/runme.log"
}
puts "=== Synthesis complete ==="

# =============================================================================
# 5. Implementation
# =============================================================================
puts "=== Starting Implementation (USB build) ==="
set_property strategy "Performance_ExplorePostRoutePhysOpt" [get_runs impl_1]
launch_runs impl_1 -jobs 4
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    error "Implementation FAILED. Check ${PROJDIR}/aegis_chip_usb.runs/impl_1/runme.log"
}
puts "=== Implementation complete ==="

# Reports
open_run impl_1
report_timing_summary -file ${ROOT}/impl/timing_summary_usb.rpt -warn_on_violation
report_utilization    -file ${ROOT}/impl/utilization_usb.rpt

set wns [get_property STATS.WNS [get_runs impl_1]]
if {$wns < 0} {
    puts "WARNING: Timing NOT met — WNS = ${wns} ns"
} else {
    puts "=== Timing MET — WNS = ${wns} ns ==="
}

# =============================================================================
# 6. Write bitstream
# =============================================================================
file mkdir ${ROOT}/impl

puts "=== Writing bitstream ==="
write_bitstream -force ${ROOT}/impl/aegis_chip_usb.bit
write_debug_probes  -force ${ROOT}/impl/aegis_chip_usb.ltx

puts ""
puts "============================================================"
puts "  Bitstream : ${ROOT}/impl/aegis_chip_usb.bit"
puts "  Program   : open_hw_manager → Connect → Program Device"
puts "============================================================"

close_project
exit
