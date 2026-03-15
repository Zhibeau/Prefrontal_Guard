`timescale 1ns / 1ps

module eth_smoke_top (
    input  wire sys_clk_p,
    input  wire sys_clk_n,
    input  wire reset_n,

    input  wire uart_rxd,
    output wire uart_txd,

    input  wire vauxp_ecg,
    input  wire vauxn_ecg,
    input  wire vauxp_eda,
    input  wire vauxn_eda,

    input  wire [3:0] key_n,
    output wire [3:0] led_n,

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

wire sys_clk;
wire [31:0] pack_total_len;
wire [1:0]  speed;
wire        link;
wire        erxdv;
wire [7:0]  erxd;
wire        mac_tx_en;
wire [7:0]  mac_txd;
wire        e_rst_n;
wire        gmii_clk = e_rxc;

reg [25:0] tx_activity_stretch = 26'd0;
reg [25:0] link_activity_stretch = 26'd0;

IBUFDS #(
    .DIFF_TERM    ("FALSE"),
    .IBUF_LOW_PWR ("TRUE"),
    .IOSTANDARD   ("DIFF_SSTL15")
) u_ibufds (
    .I  (sys_clk_p),
    .IB (sys_clk_n),
    .O  (sys_clk)
);

assign e_gtxc  = gmii_clk;
assign e_reset = 1'b1;
assign e_txer  = 1'b0;
assign uart_txd = 1'b1;

always @(posedge gmii_clk or negedge reset_n) begin
    if (!reset_n) begin
        tx_activity_stretch   <= 26'd0;
        link_activity_stretch <= 26'd0;
    end else begin
        if (e_txen) begin
            tx_activity_stretch <= 26'd62_500_000;
        end else if (tx_activity_stretch != 26'd0) begin
            tx_activity_stretch <= tx_activity_stretch - 1'b1;
        end

        if (link) begin
            link_activity_stretch <= 26'd62_500_000;
        end else if (link_activity_stretch != 26'd0) begin
            link_activity_stretch <= link_activity_stretch - 1'b1;
        end
    end
end

assign led_n[0] = ~link;
assign led_n[1] = ~e_rst_n;
assign led_n[2] = ~(tx_activity_stretch != 26'd0);
assign led_n[3] = ~(link_activity_stretch != 26'd0);

smi_config #(
    .REF_CLK (200),
    .MDC_CLK (500)
) u_smi_config (
    .clk    (sys_clk),
    .rst_n  (reset_n),
    .mdc    (e_mdc),
    .mdio   (e_mdio),
    .speed  (speed),
    .link   (link),
    .led    ()
);

mac_test u_mac_test (
    .rst_n         (e_rst_n),
    .pack_total_len(pack_total_len),
    .gmii_tx_clk   (gmii_clk),
    .gmii_rx_clk   (gmii_clk),
    .gmii_rx_dv    (erxdv),
    .gmii_rxd      (erxd),
    .gmii_tx_en    (mac_tx_en),
    .gmii_txd      (mac_txd)
);

gmii_arbi u_gmii_arbi (
    .clk            (gmii_clk),
    .rst_n          (reset_n),
    .speed          (speed),
    .link           (link),
    .pack_total_len (pack_total_len),
    .e_rst_n        (e_rst_n),
    .gmii_rx_dv     (e_rxdv),
    .gmii_rxd       (e_rxd),
    .gmii_tx_en     (mac_tx_en),
    .gmii_txd       (mac_txd),
    .e_rx_dv        (erxdv),
    .e_rxd          (erxd),
    .e_tx_en        (e_txen),
    .e_txd          (e_txd)
);

wire _unused_ok = &{1'b0, uart_rxd, vauxp_ecg, vauxn_ecg, vauxp_eda, vauxn_eda,
                    key_n[0], key_n[1], key_n[2], key_n[3], e_rxer, e_txc};

endmodule
