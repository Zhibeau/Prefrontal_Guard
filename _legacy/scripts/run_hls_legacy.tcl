open_project aegis_ml_prj
set_top aegis_ml_inference_pipeline
add_files aegis_fpga_ml_inference.cpp
add_files rf_biological_arousal.h
open_solution "solution1" -flow_target vivado
set_part {xc7a100tcsg324-1}
create_clock -period 10 -name default
csynth_design
export_design -format ip_catalog
exit
