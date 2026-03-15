set ROOT [file normalize [file dirname [info script]]/..]
set XPR  ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.xpr

open_project ${XPR}
update_compile_order -fileset sources_1
reset_run synth_1
reset_run impl_1
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    error "Vendor demo implementation FAILED. Check eth_test.runs/impl_1/runme.log"
}

open_run impl_1
report_timing_summary -file ${ROOT}/ethernet_test/rgmii_ethernet/eth_test.runs/impl_1/timing_summary_post_fix.rpt -warn_on_violation
close_project
exit
