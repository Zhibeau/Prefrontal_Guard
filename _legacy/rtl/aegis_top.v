// =============================================================================
// aegis_top.v — Aegis-Chip UART-to-UDP integration top-level
// =============================================================================
//
// Active bring-up pipeline:
//
//   Raspberry Pi / host ──USB/CP2102 UART──► uart_rx
//                                          ► aegis_uart_bridge
//                                          ► aegis_shield
//                                          ► aegis_vector_to_ram
//                                          ► UDP TX over GMII Ethernet
//
// Notes:
//   * UART input is 16 × INT16, little-endian, 32 bytes/frame.
//   * anxiety_level is currently driven by the push-buttons for deterministic
//     bring-up and hardware validation.
//   * Ethernet payload is the 32-byte steering vector in little-endian INT16.
// =============================================================================

`timescale 1ns / 1ps

module aegis_top (
    // ---- Differential 200 MHz system clock (AX7102 Bank34) ------------------
    input  wire sys_clk_p,
    input  wire sys_clk_n,

    // ---- Active-low reset button (Bank34, T6) --------------------------------
    input  wire reset_n,

    // ---- UART physical interface (Bank13, CP2102GM) -----------------------
    input  wire uart_rxd,       // Y12
    output wire uart_txd,       // Y11

    // ---- XADC analog inputs (route to VP/VN and VAUXP/VAUXN pads via XDC) --
    input  wire vauxp_ecg,      // XADC_VP  = L10 (CON1 PIN53)
    input  wire vauxn_ecg,      // XADC_VN  = M9  (CON1 PIN51)
    input  wire vauxp_eda,      // VAUXP[1] = TBD (confirm from FGG484 package)
    input  wire vauxn_eda,      // VAUXN[1] = TBD

    // ---- Expansion-board user buttons (active-low, 3.3 V) -------------------
    // KEY1=B18, KEY2=B17, KEY3=A16, KEY4=A15  (Bank14/15, LVCMOS33)
    // Binary encoding: KEY1=MSB(×512), KEY2(×256), KEY3(×128), KEY4=LSB(×64)
    input  wire [3:0] key_n,

    // ---- Expansion-board user LEDs (active-low, 3.3 V) ----------------------
    // LED1=C17, LED2=D17, LED3=V20, LED4=U20  (Bank15/14, LVCMOS33)
    // Each LED mirrors its corresponding button: ON when pressed
    output wire [3:0] led_n,

    // ---- Ethernet GMII interface (RTL8211EG PHY, Bank13/14, LVCMOS33) ------
    output wire       e_reset,
    output wire       e_mdc,
    inout  wire       e_mdio,
    input  wire       e_rxc,
    input  wire       e_rxdv,
    input  wire       e_rxer,
    input  wire [7:0] e_rxd,
    input  wire       e_txc,
    output wire       e_gtxc,
    output wire       e_txen,
    output wire       e_txer,
    output wire [7:0] e_txd
);

// =============================================================================
// 1. Clock — IBUFDS: 200 MHz LVDS differential → single-ended
//    Then divide by 2 for the UART/control/shield domain.
// =============================================================================
wire sys_clk_200m;
wire sys_clk_core;
reg  sys_clk_div2 = 1'b0;

IBUFDS #(
    .DIFF_TERM    ("FALSE"),
    .IBUF_LOW_PWR ("TRUE"),
    .IOSTANDARD   ("DIFF_SSTL15") // Bank34 Vcco=1.5V (DDR3 bank); SiTime SiT9102 LVDS into 1.5V bank
) u_ibufds (
    .I  (sys_clk_p),
    .IB (sys_clk_n),
    .O  (sys_clk_200m)
);

always @(posedge sys_clk_200m) begin
    sys_clk_div2 <= ~sys_clk_div2;
end

BUFG u_clkdiv (
    .I (sys_clk_div2),
    .O (sys_clk_core)
);

// anxiety_level is intentionally button-driven in this burnable hardware build.
wire [15:0] anxiety_level;
wire [3:0]  btn_led_n_unused;

// =============================================================================
// 2b. Pipeline A — Button Anxiety Encoder
//     4 push-buttons → 16 discrete anxiety levels [0, 64, 128, …, 960].
//     anxiety_level = {KEY1,KEY2,KEY3,KEY4} × 64
//     LEDs mirror the debounced button state for visual confirmation.
// =============================================================================
btn_anxiety #(
    .CLK_FREQ    (100_000_000),
    .DEBOUNCE_MS (20)
) u_btn_anxiety (
    .clk          (sys_clk_core),
    .rst_n        (reset_n),
    .key_n        (key_n),
    .led_n        (btn_led_n_unused),
    .anxiety_level(anxiety_level)
);

// =============================================================================
// 3. AXI-Stream interconnect wires
//    uart_rx → aegis_uart_bridge → aegis_shield → aegis_vector_to_ram
// =============================================================================
wire [15:0] uart_axis_tdata;
wire        uart_axis_tvalid;
wire        uart_axis_tready;

wire [15:0] shield_in_tdata;
wire        shield_in_tvalid;
wire        shield_in_tready;
wire [1:0]  shield_in_tkeep = 2'b11;
wire        shield_in_tlast = 1'b0;

wire [15:0] shield_out_tdata;
wire        shield_out_tvalid;
wire        shield_out_tready;
wire [1:0]  shield_out_tkeep;
wire        shield_out_tlast;

wire        frame_captured;
wire        frame_replayed;
wire        vector_frame_written;

// =============================================================================
// 4. Pipeline A — UART RX (8-bit deserialiser + 2-byte→16-bit assembler)
// =============================================================================
uart_rx #(
    .CLK_FREQ     (100_000_000),
    .BAUD_RATE    (115_200)
) u_uart_rx (
    .clk            (sys_clk_core),
    .rst_n          (reset_n),
    .rxd            (uart_rxd),
    .m_axis_tdata   (uart_axis_tdata),
    .m_axis_tvalid  (uart_axis_tvalid),
    .m_axis_tready  (uart_axis_tready)
);

// =============================================================================
// 5. Bridge UART frames into deterministic 16-word bursts for aegis_shield.
// =============================================================================
aegis_uart_bridge u_uart_bridge (
    .clk            (sys_clk_core),
    .rst_n          (reset_n),
    .s_axis_tdata   (uart_axis_tdata),
    .s_axis_tvalid  (uart_axis_tvalid),
    .s_axis_tready  (uart_axis_tready),
    .m_axis_tdata   (shield_in_tdata),
    .m_axis_tvalid  (shield_in_tvalid),
    .m_axis_tready  (shield_in_tready),
    .frame_captured (frame_captured),
    .frame_replayed (frame_replayed)
);

// =============================================================================
// 6. CC-CBF steering engine.
// =============================================================================
aegis_shield u_aegis_shield (
    .ap_clk         (sys_clk_core),
    .ap_rst_n       (reset_n),
    .anx_in         (anxiety_level),
    .x_t_TDATA      (shield_in_tdata),
    .x_t_TVALID     (shield_in_tvalid),
    .x_t_TREADY     (shield_in_tready),
    .x_t_TKEEP      (shield_in_tkeep),
    .x_t_TSTRB      (shield_in_tkeep),
    .x_t_TLAST      (shield_in_tlast),
    .u_t_TDATA      (shield_out_tdata),
    .u_t_TVALID     (shield_out_tvalid),
    .u_t_TREADY     (shield_out_tready),
    .u_t_TKEEP      (shield_out_tkeep),
    .u_t_TLAST      (shield_out_tlast)
);

// =============================================================================
// 7. Capture the steering vector into a dual-port RAM image for UDP TX.
// =============================================================================
aegis_vector_to_ram u_vector_to_ram (
    .clk            (sys_clk_core),
    .rst_n          (reset_n),
    .s_axis_tdata   (shield_out_tdata),
    .s_axis_tvalid  (shield_out_tvalid),
    .s_axis_tready  (shield_out_tready),
    .ram_wr_en      (ram_wr_en),
    .ram_wr_addr    (ram_wr_addr),
    .ram_wr_data    (ram_wr_data),
    .frame_written  (vector_frame_written)
);

assign uart_txd = 1'b1;

// =============================================================================
// 8. PHY reset + MDIO management + GMII adaptation
// =============================================================================
reg [24:0] phy_reset_cnt;

always @(posedge sys_clk_core or negedge reset_n) begin
    if (!reset_n) begin
        phy_reset_cnt <= 25'd0;
    end else if (phy_reset_cnt < 25'd10_000_000) begin
        phy_reset_cnt <= phy_reset_cnt + 1'b1;
    end
end

wire phy_ready = (phy_reset_cnt == 25'd10_000_000);
assign e_reset = phy_ready;

wire [1:0] eth_speed;
wire       eth_link;
wire [3:0] eth_led_unused;
wire gmii_clk = e_rxc;

(* ASYNC_REG = "TRUE" *) reg phy_ready_meta = 1'b0;
(* ASYNC_REG = "TRUE" *) reg phy_ready_gmii = 1'b0;

always @(posedge gmii_clk or negedge reset_n) begin
    if (!reset_n) begin
        phy_ready_meta <= 1'b0;
        phy_ready_gmii <= 1'b0;
    end else begin
        phy_ready_meta <= phy_ready;
        phy_ready_gmii <= phy_ready_meta;
    end
end

wire gmii_logic_rst_n = reset_n & phy_ready_gmii;

smi_config #(
    .REF_CLK (125),
    .MDC_CLK (500)
) u_smi_config (
    .clk    (gmii_clk),
    .rst_n  (gmii_logic_rst_n),
    .mdc    (e_mdc),
    .mdio   (e_mdio),
    .speed  (eth_speed),
    .link   (eth_link),
    .led    (eth_led_unused)
);

assign e_gtxc = gmii_clk;

wire [31:0] ram_rd_data;
wire [8:0]  ram_rd_addr;
wire [8:0]  ram_wr_addr;
wire [31:0] ram_wr_data;
wire        ram_wr_en;

wire        udp_rx_dv;
wire [7:0]  udp_rxd;
wire        core_tx_en;
wire [7:0]  core_txd;
wire        core_tx_er;
wire        eth_core_rst_n;
wire [3:0]  udp_tx_state;

reg [25:0] tx_activity_stretch = 26'd0;

always @(posedge gmii_clk or negedge reset_n) begin
    if (!reset_n) begin
        tx_activity_stretch <= 26'd0;
    end else if (core_tx_en) begin
        tx_activity_stretch <= 26'd62_500_000;
    end else if (tx_activity_stretch != 26'd0) begin
        tx_activity_stretch <= tx_activity_stretch - 1'b1;
    end
end

assign led_n[0] = ~phy_ready;
assign led_n[1] = ~gmii_logic_rst_n;
assign led_n[2] = ~eth_link;
assign led_n[3] = ~(tx_activity_stretch != 26'd0);

gmii_arbi u_gmii_arbi (
    .clk            (gmii_clk),
    .rst_n          (gmii_logic_rst_n),
    .speed          (eth_speed),
    .link           (eth_link),
    .gmii_rx_dv     (e_rxdv),
    .gmii_rxd       (e_rxd),
    .gmii_tx_en     (core_tx_en),
    .gmii_txd       (core_txd),
    .pack_total_len (),
    .e_rst_n        (eth_core_rst_n),
    .e_rx_dv        (udp_rx_dv),
    .e_rxd          (udp_rxd),
    .e_tx_en        (e_txen),
    .e_txd          (e_txd)
);

assign e_txer = core_tx_er;

udp u_udp (
    .reset_n         (eth_core_rst_n & gmii_logic_rst_n),
    .g_clk           (gmii_clk),
    .e_rxc           (gmii_clk),
    .e_rxd           (udp_rxd),
    .e_rxdv          (udp_rx_dv),
    .e_txen          (core_tx_en),
    .e_txd           (core_txd),
    .e_txer          (core_tx_er),
    .data_o_valid    (),
    .ram_wr_data     (),
    .rx_total_length (),
    .rx_state        (),
    .rx_data_length  (),
    .ram_wr_addr     (),
    .data_receive    (),
    .ram_rd_data     (ram_rd_data),
    .tx_state        (udp_tx_state),
    .tx_data_length  (16'd40),
    .tx_total_length (16'd60),
    .ram_rd_addr     (ram_rd_addr)
);

dp_ram #(
    .DATA_WIDTH (32),
    .MEM_SIZE   (512)
) u_udp_payload_ram (
    .data       (ram_wr_data),
    .rdaddress  (ram_rd_addr),
    .rdclock    (gmii_clk),
    .wraddress  (ram_wr_addr),
    .wrclock    (sys_clk_core),
    .wren       (ram_wr_en),
    .q          (ram_rd_data)
);

wire _unused_ok = &{1'b0, vauxp_ecg, vauxn_ecg, vauxp_eda, vauxn_eda, e_txc,
                    frame_captured, frame_replayed, vector_frame_written,
                    shield_out_tkeep[0], shield_out_tkeep[1], shield_out_tlast,
                    btn_led_n_unused[0], btn_led_n_unused[1], btn_led_n_unused[2], btn_led_n_unused[3],
                    udp_tx_state[0], udp_tx_state[1], udp_tx_state[2], udp_tx_state[3]};

endmodule
