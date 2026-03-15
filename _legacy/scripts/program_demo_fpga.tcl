open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target
set_property PROGRAM.FILE {/home/midu/aegis_fpga/ethernet_test/rgmii_ethernet/eth_test.runs/impl_1/ethernet_test.bit} [get_hw_devices xc7a100t_0]
program_hw_devices [get_hw_devices xc7a100t_0]
close_hw_target
exit
