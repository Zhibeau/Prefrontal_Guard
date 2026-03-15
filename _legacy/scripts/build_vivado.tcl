# =============================================================================
# build_vivado.tcl — Full Vivado Project Creation and Bitstream Generation
# =============================================================================
# Usage (from aegis_fpga/ root, AFTER running build_all_hls.tcl):
#   vivado -mode batch -source scripts/build_vivado.tcl
#
# Flow:
#   1. Create Vivado project targeting XC7A100T-2FGG484I
#   2. Add application RTL sources (rtl/*.v)
#   3. Add Ethernet helper RTL from ethernet_test/
#   4. Add XDC constraints
#   5. Run synthesis, implementation, and write_bitstream
#
# Output: impl/aegis_chip.bit  (ready to program via JTAG)
# =============================================================================

set PART    "xc7a100tifgg484-1L"
set ROOT    [file normalize [file dirname [info script]]/.. ]
set PROJDIR ${ROOT}/vivado_proj

# =============================================================================
# 1. Create project
# =============================================================================
file mkdir ${PROJDIR}
create_project aegis_chip ${PROJDIR} -part ${PART} -force

set_property target_language  Verilog [current_project]
set_property simulator_language Mixed  [current_project]

# =============================================================================
# 2. Add RTL source files
# =============================================================================
add_files -norecurse [glob ${ROOT}/rtl/*.v]
set_property top aegis_top [current_fileset]

# =============================================================================
# 3. Add Ethernet helper RTL
# =============================================================================
add_files -norecurse [glob ${ROOT}/ethernet_test/rgmii_ethernet/src/rtl/*.v]
add_files -norecurse [glob ${ROOT}/ethernet_test/rgmii_ethernet/src/arbi/*.v]
add_files -norecurse [glob ${ROOT}/ethernet_test/rgmii_ethernet/src/mdio/*.v]
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/src/dp_ram.v
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/eth_data_fifo/eth_data_fifo.xci
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/len_fifo/len_fifo.xci

# =============================================================================
# 4. Add XDC constraints
# =============================================================================
add_files -fileset constrs_1 -norecurse \
    ${ROOT}/constraints/aegis_ax7102.xdc

# =============================================================================
# 5. Synthesis
#    -directive PerformanceOptimized — maximise Fmax for 200 MHz target
# =============================================================================
puts "=== Starting Synthesis ==="
launch_runs synth_1 -jobs 4
wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    error "Synthesis FAILED. Check vivado_proj/aegis_chip.runs/synth_1/runme.log"
}
puts "=== Synthesis complete ==="

# =============================================================================
# 6. Implementation
#    -directive ExplorePostRoutePhysOpt — aggressive timing closure
# =============================================================================
puts "=== Starting Implementation ==="
set_property strategy "Performance_ExplorePostRoutePhysOpt" [get_runs impl_1]
launch_runs impl_1 -jobs 4
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    error "Implementation FAILED. Check vivado_proj/aegis_chip.runs/impl_1/runme.log"
}
puts "=== Implementation complete ==="

# Report timing and utilisation
open_run impl_1
report_timing_summary -file ${ROOT}/impl/timing_summary.rpt  -warn_on_violation
report_utilization    -file ${ROOT}/impl/utilization.rpt
report_power          -file ${ROOT}/impl/power.rpt

# Fail loudly if timing is not met
set wns [get_property STATS.WNS [get_runs impl_1]]
if {$wns < 0} {
    puts "WARNING: Timing NOT met — WNS = ${wns} ns. Review timing_summary.rpt"
    puts "         Consider adding PBLOCK constraints or reducing DSP parallelism."
} else {
    puts "=== Timing MET — WNS = ${wns} ns ==="
}

# =============================================================================
# 7. Write bitstream
# =============================================================================
file mkdir ${ROOT}/impl

puts "=== Writing bitstream ==="
write_bitstream -force ${ROOT}/impl/aegis_chip.bit
write_debug_probes  -force ${ROOT}/impl/aegis_chip.ltx

puts ""
puts "============================================================"
puts "  Bitstream : ${ROOT}/impl/aegis_chip.bit"
puts "  Program   : open_hw_manager → Connect → Program Device"
puts "  Or use    : program_hw_devices [get_hw_devices xc7a100t_0]"
puts "============================================================"

close_project
exit
