# Nexys4 DDR xdc
# LED [7:0]
############################
# On-board led             #
############################
set_property -dict {PACKAGE_PIN H17 IOSTANDARD LVCMOS33} [get_ports {led_pins[0]}]
set_property -dict {PACKAGE_PIN K15 IOSTANDARD LVCMOS33} [get_ports {led_pins[1]}]
set_property -dict {PACKAGE_PIN J13 IOSTANDARD LVCMOS33} [get_ports {led_pins[2]}]
set_property -dict {PACKAGE_PIN N14 IOSTANDARD LVCMOS33} [get_ports {led_pins[3]}]
set_property -dict {PACKAGE_PIN R18 IOSTANDARD LVCMOS33} [get_ports {led_pins[4]}]
set_property -dict {PACKAGE_PIN V17 IOSTANDARD LVCMOS33} [get_ports {led_pins[5]}]
set_property -dict {PACKAGE_PIN U17 IOSTANDARD LVCMOS33} [get_ports {led_pins[6]}]
set_property -dict {PACKAGE_PIN U16 IOSTANDARD LVCMOS33} [get_ports {led_pins[7]}]

# CLK source 100 MHz
set_property -dict {PACKAGE_PIN E3 IOSTANDARD LVCMOS33} [get_ports clk_pin]

# BTNU
set_property -dict {PACKAGE_PIN M18 IOSTANDARD LVCMOS33} [get_ports btn_pin]

# RXD UART
set_property -dict {PACKAGE_PIN C4 IOSTANDARD LVCMOS33} [get_ports rxd_pin]

# Reset - BTNC
set_property -dict {PACKAGE_PIN N17 IOSTANDARD LVCMOS33} [get_ports rst_pin]

set_switching_activity -deassert_resets 




create_pblock pblock_meta_harden_btn_i0
add_cells_to_pblock [get_pblocks pblock_meta_harden_btn_i0] [get_cells -quiet [list meta_harden_btn_i0]]
resize_pblock [get_pblocks pblock_meta_harden_btn_i0] -add {SLICE_X10Y112:SLICE_X33Y135}
resize_pblock [get_pblocks pblock_meta_harden_btn_i0] -add {DSP48_X0Y46:DSP48_X0Y53}
