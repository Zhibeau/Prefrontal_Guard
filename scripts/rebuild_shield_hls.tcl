# =============================================================================
# rebuild_shield_hls.tcl — Rebuild aegis_shield HLS IP only
# =============================================================================
# Usage (from aegis_fpga/ root):
#   vitis-run --mode hls --tcl scripts/rebuild_shield_hls.tcl
# =============================================================================

set PART   "xc7a100tifgg484-1L"
set ROOT   [file normalize [file dirname [info script]]/.. ]
set IPREPO ${ROOT}/ip_repo

open_project  aegis_shield_prj
set_top       aegis_shield
add_files     ${ROOT}/hls/aegis_shield/aegis_shield.cpp
add_files     ${ROOT}/hls/aegis_shield/aegis_shield.h
open_solution "sol1" -flow_target vivado
set_part      $PART
create_clock  -period 5 -name default

csynth_design

export_design -format ip_catalog \
              -output "${IPREPO}/aegis_shield_ip" \
              -evaluate verilog

close_project

puts ""
puts "=== aegis_shield HLS IP rebuilt → ${IPREPO}/aegis_shield_ip ==="
puts "=== Now run: vivado -mode batch -source scripts/build_vivado_usb.tcl ==="

exit
