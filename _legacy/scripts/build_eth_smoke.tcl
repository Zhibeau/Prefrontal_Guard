# =============================================================================
# build_eth_smoke.tcl — Minimal Ethernet smoke-test build
# =============================================================================
# Purpose:
#   Build an isolated Ethernet-only diagnostic image that bypasses the current
#   UART/shield/UDP integration and instead uses the vendor-style MAC path.
#   This helps determine whether the issue is in our application integration or
#   in the GMII/PHY TX path itself.
# =============================================================================

set PART    "xc7a100tifgg484-1L"
set ROOT    [file normalize [file dirname [info script]]/.. ]
set PROJDIR ${ROOT}/vivado_proj_eth_smoke

file mkdir ${PROJDIR}
create_project aegis_eth_smoke ${PROJDIR} -part ${PART} -force

set_property target_language  Verilog [current_project]
set_property simulator_language Mixed  [current_project]

# ----------------------------------------------------------------------------
# Top-level diagnostic RTL
# ----------------------------------------------------------------------------
add_files -norecurse ${ROOT}/rtl/eth_smoke_top.v
set_property top eth_smoke_top [current_fileset]

# ----------------------------------------------------------------------------
# GMII/MDIO support used by the smoke-test top
# ----------------------------------------------------------------------------
add_files -norecurse [glob ${ROOT}/ethernet_test/rgmii_ethernet/src/arbi/*.v]
add_files -norecurse [glob ${ROOT}/ethernet_test/rgmii_ethernet/src/mdio/*.v]

# ----------------------------------------------------------------------------
# Vendor MAC path used for isolated TX validation
# ----------------------------------------------------------------------------
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/mac_top.v
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/mac_test.v
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/arp_cache.v
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/icmp_reply.v
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/crc.v
add_files -norecurse [glob ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/tx/*.v]
add_files -norecurse [glob ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/mac/rx/*.v]

# ----------------------------------------------------------------------------
# Required IP blocks for the MAC path and GMII buffers
# ----------------------------------------------------------------------------
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/eth_data_fifo/eth_data_fifo.xci
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/len_fifo/len_fifo.xci
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/udp_tx_data_fifo/udp_tx_data_fifo.xci
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/udp_checksum_fifo/udp_checksum_fifo.xci
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/udp_rx_ram_8_2048/udp_rx_ram_8_2048.xci
add_files -norecurse ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.srcs/sources_1/ip/icmp_rx_ram_8_256/icmp_rx_ram_8_256.xci

# ----------------------------------------------------------------------------
# Constraints
# ----------------------------------------------------------------------------
add_files -fileset constrs_1 -norecurse ${ROOT}/constraints/aegis_ax7102.xdc

# ----------------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------------
puts "=== Starting Ethernet smoke-test synthesis ==="
launch_runs synth_1 -jobs 4
wait_on_run synth_1
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    error "Synthesis FAILED. Check vivado_proj_eth_smoke/aegis_eth_smoke.runs/synth_1/runme.log"
}
puts "=== Synthesis complete ==="

puts "=== Starting Ethernet smoke-test implementation ==="
set_property strategy "Performance_ExplorePostRoutePhysOpt" [get_runs impl_1]
launch_runs impl_1 -jobs 4
wait_on_run impl_1
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    error "Implementation FAILED. Check vivado_proj_eth_smoke/aegis_eth_smoke.runs/impl_1/runme.log"
}
puts "=== Implementation complete ==="

open_run impl_1
report_timing_summary -file ${ROOT}/impl/timing_eth_smoke.rpt -warn_on_violation
report_utilization    -file ${ROOT}/impl/utilization_eth_smoke.rpt

set wns [get_property STATS.WNS [get_runs impl_1]]
if {$wns < 0} {
    puts "WARNING: Timing NOT met — WNS = ${wns} ns"
} else {
    puts "=== Timing MET — WNS = ${wns} ns ==="
}

file mkdir ${ROOT}/impl
puts "=== Writing Ethernet smoke-test bitstream ==="
write_bitstream -force ${ROOT}/impl/aegis_chip.bit
puts "=== Smoke-test bitstream written to ${ROOT}/impl/aegis_chip.bit ==="

close_project
exit
