// =============================================================================
// aegis_top_usb.v — Aegis-Chip USB integration top-level
// =============================================================================
//
// Active pipeline:
//
//   Host PC
//     ↕ USB (CY68013A EZ-USB FX2LP, 16-bit parallel slave FIFO)
//   aegis_usb_bridge  ← EP2 (host→FPGA): 16 × INT16 input vector
//                     → EP6 (FPGA→host): 16 × INT16 steering vector
//     ↕ AXI-Stream
//   aegis_uart_bridge (batches/replays the 16-word frame)
//     ↕ AXI-Stream
//   aegis_shield      (current workspace version is a debug stub)
//
// Frame format: 16 × INT16, little-endian, 32 bytes.
//
// Temporary bring-up LED mapping (active-low):
//   LED1 = raw usb_flaga   (EP2 not empty from FX2LP)
//   LED2 = synced FLAGA    (as seen inside aegis_usb_bridge)
//   LED3 = bridge left IDLE state
//   LED4 = EP2 frame fully read into usb_bridge (sticky)
// =============================================================================

`timescale 1ns / 1ps

module aegis_top_usb (
    // ---- Differential 200 MHz system clock (Bank34) -------------------------
    input  wire        sys_clk_p,
    input  wire        sys_clk_n,

    // ---- Active-low reset button (Bank34, T6) -------------------------------
    input  wire        reset_n,

    // ---- XADC analog inputs (currently unused in USB build) -----------------
    input  wire        vauxp_ecg,
    input  wire        vauxn_ecg,
    input  wire        vauxp_eda,
    input  wire        vauxn_eda,

    // ---- Expansion-board user buttons (active-low, LVCMOS33) ---------------
    input  wire [3:0]  key_n,

    // ---- Expansion-board user LEDs (active-low, LVCMOS33) ------------------
    output wire [3:0]  led_n,

    // ---- CY68013A FX2LP USB slave-FIFO interface ----------------------------
    output wire [1:0]  usb_fifoaddr,
    output wire        usb_slcs,
    output wire        usb_sloe,
    output wire        usb_slrd,
    output wire        usb_slwr,
    inout  wire [15:0] usb_fd,
    input  wire        usb_flaga,
    input  wire        usb_flagb,
    input  wire        usb_flagc
);

// =============================================================================
// 1. Clock — IBUFDS: 200 MHz LVDS → single-ended, then ÷2 for 100 MHz core
// =============================================================================
wire sys_clk_200m;
wire sys_clk_core;
reg  sys_clk_div2 = 1'b0;

IBUFDS #(
    .DIFF_TERM    ("FALSE"),
    .IBUF_LOW_PWR ("TRUE"),
    .IOSTANDARD   ("DIFF_SSTL15")
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

// =============================================================================
// 2. Button anxiety encoder — still used to drive anx_in
// =============================================================================
wire [15:0] anxiety_level;
wire [3:0]  btn_led_n;

btn_anxiety #(
    .CLK_FREQ    (100_000_000),
    .DEBOUNCE_MS (20)
) u_btn_anxiety (
    .clk           (sys_clk_core),
    .rst_n         (reset_n),
    .key_n         (key_n),
    .led_n         (btn_led_n),
    .anxiety_level (anxiety_level)
);

// =============================================================================
// 3. AXI-Stream interconnect wires
// =============================================================================
wire [15:0] usb_rx_tdata;
wire        usb_rx_tvalid;
wire        usb_rx_tready;

wire [15:0] shield_in_tdata;
wire        shield_in_tvalid;
wire        shield_in_tready;
wire [1:0]  shield_in_tkeep  = 2'b11;
wire        shield_in_tlast  = 1'b0;

wire [15:0] shield_out_tdata;
wire        shield_out_tvalid;
wire        shield_out_tready;
wire [1:0]  shield_out_tkeep;
wire        shield_out_tlast;

wire        frame_captured;
wire        frame_replayed;
wire        rx_frame_done;
wire        tx_frame_done;
wire        usb_flaga_sync_dbg;
wire        usb_bridge_left_idle;

reg dbg_rx_seen;

always @(posedge sys_clk_core or negedge reset_n) begin
    if (!reset_n) begin
        dbg_rx_seen <= 1'b0;
    end else if (rx_frame_done) begin
        dbg_rx_seen <= 1'b1;
    end
end

assign led_n[0] = ~usb_flaga;
assign led_n[1] = ~usb_flaga_sync_dbg;
assign led_n[2] = ~usb_bridge_left_idle;
assign led_n[3] = ~dbg_rx_seen;

// =============================================================================
// 4. CY68013A FX2LP USB slave-FIFO bridge
// =============================================================================
aegis_usb_bridge #(
    .WORDS_PER_FRAME (16),
    .OE_SETUP_CYCS   (25),
    .RD_PULSE_CYCS   (35),
    .RD_HOLD_CYCS    (19),
    .WR_SETUP_CYCS   (10),
    .WR_PULSE_CYCS   (35),
    .WR_HOLD_CYCS    (19)
) u_usb_bridge (
    .clk            (sys_clk_core),
    .rst_n          (reset_n),

    .usb_fifoaddr   (usb_fifoaddr),
    .usb_slcs       (usb_slcs),
    .usb_sloe       (usb_sloe),
    .usb_slrd       (usb_slrd),
    .usb_slwr       (usb_slwr),
    .usb_fd         (usb_fd),
    .usb_flaga      (usb_flaga),
    .usb_flagb      (usb_flagb),
    .usb_flagc      (usb_flagc),

    .m_axis_tdata   (usb_rx_tdata),
    .m_axis_tvalid  (usb_rx_tvalid),
    .m_axis_tready  (usb_rx_tready),

    .s_axis_tdata   (shield_out_tdata),
    .s_axis_tvalid  (shield_out_tvalid),
    .s_axis_tready  (shield_out_tready),

    .rx_frame_done  (rx_frame_done),
    .tx_frame_done  (tx_frame_done),
    .dbg_flaga_sync (usb_flaga_sync_dbg),
    .dbg_left_idle  (usb_bridge_left_idle)
);

// =============================================================================
// 5. UART bridge — batches 16 incoming words into a deterministic burst
// =============================================================================
aegis_uart_bridge u_uart_bridge (
    .clk            (sys_clk_core),
    .rst_n          (reset_n),
    .s_axis_tdata   (usb_rx_tdata),
    .s_axis_tvalid  (usb_rx_tvalid),
    .s_axis_tready  (usb_rx_tready),
    .m_axis_tdata   (shield_in_tdata),
    .m_axis_tvalid  (shield_in_tvalid),
    .m_axis_tready  (shield_in_tready),
    .frame_captured (frame_captured),
    .frame_replayed (frame_replayed)
);

// =============================================================================
// 6. Shield / compute block
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
// 7. Tie off unused signals
// =============================================================================
wire _unused_ok = &{1'b0,
    vauxp_ecg, vauxn_ecg, vauxp_eda, vauxn_eda,
    frame_captured, frame_replayed, tx_frame_done,
    shield_out_tkeep[0], shield_out_tkeep[1], shield_out_tlast,
    btn_led_n[0], btn_led_n[1], btn_led_n[2], btn_led_n[3]};

endmodule
