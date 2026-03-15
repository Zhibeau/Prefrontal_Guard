# =============================================================================
# program_fpga_usb.tcl — Program AX7102 with aegis_chip_usb.bit
# =============================================================================
# Usage:
#   vivado -mode batch -source scripts/program_fpga_usb.tcl
# =============================================================================

set ROOT    [file normalize [file dirname [info script]]/.. ]
set BITFILE ${ROOT}/impl/aegis_chip_usb.bit

if {![file exists $BITFILE]} {
    error "Bitstream not found: $BITFILE — run build_vivado_usb.tcl first"
}

open_hw_manager
connect_hw_server -allow_non_jtag

# Auto-detect the first target (AX7102 presents as xc7a100t)
open_hw_target

set dev [lindex [get_hw_devices] 0]
current_hw_device $dev
refresh_hw_device -update_hw_probes false $dev

set_property PROGRAM.FILE $BITFILE $dev
program_hw_devices $dev

puts ""
puts "=== FPGA programmed with: $BITFILE ==="
puts "=== Board should be running the USB CC-CBF build ==="

close_hw_target
disconnect_hw_server
close_hw_manager
exit
